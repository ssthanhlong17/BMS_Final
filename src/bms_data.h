#ifndef BMS_DATA_H
#define BMS_DATA_H

#include <ArduinoJson.h>
#include "soc_estimator.h"
#include "soh_estimator.h"

const int NUM_CELLS = 4;

// ==================== NGƯỠNG BẢO VỆ LiFePO4 ====================

// Điện áp Cell
#define CELL_UV_WARNING 3.0         // Warning: Điện áp thấp
#define CELL_UV_CRITICAL 2.5        // Critical: Ngắt xả
#define CELL_UV_RECOVERY 2.9        // Hết cảnh báo critical

#define CELL_OV_WARNING 3.45        // Warning: Điện áp cao
#define CELL_OV_CRITICAL 3.65       // Critical: Ngắt sạc
#define CELL_OV_RECOVERY 3.4        // Hết cảnh báo critical

// Dòng điện
#define CURRENT_DISCHARGE_WARNING -4.0    // Warning: Dòng xả cao (A)
#define CURRENT_DISCHARGE_CRITICAL -6.0   // Critical: Ngắt xả (A)
#define CURRENT_DISCHARGE_RECOVERY -3.5   // Hết cảnh báo critical (A)

#define CURRENT_CHARGE_WARNING 1.4        // Warning: Dòng sạc cao (A)
#define CURRENT_CHARGE_CRITICAL 2.0       // Critical: Ngắt sạc (A)
#define CURRENT_CHARGE_RECOVERY 0.8       // Hết cảnh báo critical (A)

// Nhiệt độ khi XẢ
#define TEMP_DISCHARGE_CRITICAL_HIGH 60.0     // Critical: Nhiệt độ cao khi xả (°C)
#define TEMP_DISCHARGE_CRITICAL_LOW -10.0     // Critical: Nhiệt độ thấp khi xả (°C)
#define TEMP_DISCHARGE_WARNING_HIGH 55.0      // Warning: Nhiệt độ cao khi xả (°C)
#define TEMP_DISCHARGE_WARNING_LOW 0.0        // Warning: Nhiệt độ thấp khi xả (°C)
#define TEMP_DISCHARGE_RECOVERY_HIGH 50.0     // Hết cảnh báo critical cao (°C)
#define TEMP_DISCHARGE_RECOVERY_LOW -8.0      // Hết cảnh báo critical thấp (°C)

// Nhiệt độ khi SẠC
#define TEMP_CHARGE_CRITICAL_HIGH 45.0        // Critical: Nhiệt độ cao khi sạc (°C)
#define TEMP_CHARGE_CRITICAL_LOW 0.0          // Critical: Nhiệt độ thấp khi sạc (°C)
#define TEMP_CHARGE_WARNING_HIGH 40.0         // Warning: Nhiệt độ cao khi sạc (°C)
#define TEMP_CHARGE_WARNING_LOW 5.0           // Warning: Nhiệt độ thấp khi sạc (°C)
#define TEMP_CHARGE_RECOVERY_HIGH 38.0        // Hết cảnh báo critical cao (°C)
#define TEMP_CHARGE_RECOVERY_LOW 3.0          // Hết cảnh báo critical thấp (°C)

// Balancing
#define CELL_BALANCE_START 0.3   // 300mV - Kích hoạt cân bằng
#define CELL_BALANCE_STOP 0.01   // 10mV - Ngắt cân bằng

// Tham số tính toán SOC
#define BATTERY_CAPACITY 6.0     // Ah (per cell / pack capacity)

struct BMSData {
    // Đo lường cơ bản
    float cellVoltages[NUM_CELLS];
    float packVoltage;
    float current;
    float packTemp;
    float soc;
    float soh;
    float avgCellVoltage;
    
    // Protection status - CRITICAL
    bool overVoltageAlarm;        
    bool underVoltageAlarm;       
    bool overCurrentChargeAlarm;  
    bool overCurrentDischargeAlarm; 
    bool overTempChargeAlarm;
    bool overTempDischargeAlarm;
    bool underTempChargeAlarm;
    bool underTempDischargeAlarm;
    
    // Protection status - WARNING
    bool overVoltageWarning;
    bool underVoltageWarning;
    bool overCurrentChargeWarning;
    bool overCurrentDischargeWarning;
    bool overTempChargeWarning;
    bool overTempDischargeWarning;
    bool underTempChargeWarning;
    bool underTempDischargeWarning;
    
    // Balancing
    bool balancingActive;
    bool balancingCells[NUM_CELLS];
    
    // Charging status
    bool isCharging;
    bool isDischarging;
    bool systemActive;
    
    // Runtime data
    unsigned long lastUpdateTime;
    unsigned long idleStartTime;
    float totalCycles;
    float remainingCapacity;
};

BMSData bmsData;

// SOC & SOH Estimator instances (sử dụng từ main)
extern SOCEstimator soc;
extern SOHEstimator soh;
extern bool socInitialized;
extern bool sohInitialized;

// ============ HELPER FUNCTIONS ============

const char* statusToString(bool alarm) {
    return alarm ? "alarm" : "normal";
}

// Kiểm tra balancing cần thiết với hysteresis
void checkBalancing() {
    float maxV = bmsData.cellVoltages[0];
    float minV = bmsData.cellVoltages[0];
    int maxIdx = 0;
    
    for (int i = 1; i < NUM_CELLS; i++) {
        if (bmsData.cellVoltages[i] > maxV) {
            maxV = bmsData.cellVoltages[i];
            maxIdx = i;
        }
        if (bmsData.cellVoltages[i] < minV) {
            minV = bmsData.cellVoltages[i];
        }
    }
    
    float diff = maxV - minV;
    
    // Hysteresis: Kích hoạt ở 0.3V, ngắt ở 0.01V
    if (!bmsData.balancingActive && diff >= CELL_BALANCE_START) {
        // Bắt đầu cân bằng
        bmsData.balancingActive = true;
        for (int i = 0; i < NUM_CELLS; i++) {
            if (bmsData.cellVoltages[i] >= (maxV - 0.01)) {
                bmsData.balancingCells[i] = true;
            } else {
                bmsData.balancingCells[i] = false;
            }
        }
    } else if (bmsData.balancingActive && diff <= CELL_BALANCE_STOP) {
        // Ngắt cân bằng
        bmsData.balancingActive = false;
        for (int i = 0; i < NUM_CELLS; i++) {
            bmsData.balancingCells[i] = false;
        }
    }
    // Nếu đang cân bằng và diff nằm giữa 0.01V - 0.3V → giữ nguyên trạng thái
}

// Kiểm tra protection với hysteresis
void checkProtection() {
    // ===== 1. ĐIỆN ÁP CELL - WARNING & CRITICAL =====
    bmsData.underVoltageWarning = false;
    bmsData.underVoltageAlarm = false;
    bmsData.overVoltageWarning = false;
    bmsData.overVoltageAlarm = false;
    
    for (int i = 0; i < NUM_CELLS; i++) {
        // Under Voltage
        if (bmsData.cellVoltages[i] < CELL_UV_WARNING) {
            bmsData.underVoltageWarning = true;
        }
        if (bmsData.cellVoltages[i] < CELL_UV_CRITICAL) {
            bmsData.underVoltageAlarm = true;
        }
        // Recovery từ critical
        if (bmsData.underVoltageAlarm && bmsData.cellVoltages[i] > CELL_UV_RECOVERY) {
            bmsData.underVoltageAlarm = false;
        }
        
        // Over Voltage
        if (bmsData.cellVoltages[i] > CELL_OV_WARNING) {
            bmsData.overVoltageWarning = true;
        }
        if (bmsData.cellVoltages[i] > CELL_OV_CRITICAL) {
            bmsData.overVoltageAlarm = true;
        }
        // Recovery từ critical
        if (bmsData.overVoltageAlarm && bmsData.cellVoltages[i] < CELL_OV_RECOVERY) {
            bmsData.overVoltageAlarm = false;
        }
    }
    
    // ===== 2. DÒNG ĐIỆN - WARNING & CRITICAL =====
    // Dòng xả (current < 0)
    bmsData.overCurrentDischargeWarning = false;
    bmsData.overCurrentDischargeAlarm = false;
    
    if (bmsData.isDischarging) {
        if (bmsData.current < CURRENT_DISCHARGE_WARNING) {
            bmsData.overCurrentDischargeWarning = true;
        }
        if (bmsData.current < CURRENT_DISCHARGE_CRITICAL) {
            bmsData.overCurrentDischargeAlarm = true;
        }
        // Recovery
        if (bmsData.overCurrentDischargeAlarm && bmsData.current > CURRENT_DISCHARGE_RECOVERY) {
            bmsData.overCurrentDischargeAlarm = false;
        }
    }
    
    // Dòng sạc (current > 0)
    bmsData.overCurrentChargeWarning = false;
    bmsData.overCurrentChargeAlarm = false;
    
    if (bmsData.isCharging) {
        if (bmsData.current > CURRENT_CHARGE_WARNING) {
            bmsData.overCurrentChargeWarning = true;
        }
        if (bmsData.current > CURRENT_CHARGE_CRITICAL) {
            bmsData.overCurrentChargeAlarm = true;
        }
        // Recovery
        if (bmsData.overCurrentChargeAlarm && bmsData.current < CURRENT_CHARGE_RECOVERY) {
            bmsData.overCurrentChargeAlarm = false;
        }
    }
    
    // ===== 3. NHIỆT ĐỘ KHI XẢ - WARNING & CRITICAL =====
    bmsData.overTempDischargeWarning = false;
    bmsData.overTempDischargeAlarm = false;
    bmsData.underTempDischargeWarning = false;
    bmsData.underTempDischargeAlarm = false;
    
    if (bmsData.isDischarging) {
        // Nhiệt độ cao
        if (bmsData.packTemp > TEMP_DISCHARGE_WARNING_HIGH) {
            bmsData.overTempDischargeWarning = true;
        }
        if (bmsData.packTemp >= TEMP_DISCHARGE_CRITICAL_HIGH) {
            bmsData.overTempDischargeAlarm = true;
        }
        // Recovery
        if (bmsData.overTempDischargeAlarm && bmsData.packTemp < TEMP_DISCHARGE_RECOVERY_HIGH) {
            bmsData.overTempDischargeAlarm = false;
        }
        
        // Nhiệt độ thấp
        if (bmsData.packTemp < TEMP_DISCHARGE_WARNING_LOW) {
            bmsData.underTempDischargeWarning = true;
        }
        if (bmsData.packTemp <= TEMP_DISCHARGE_CRITICAL_LOW) {
            bmsData.underTempDischargeAlarm = true;
        }
        // Recovery
        if (bmsData.underTempDischargeAlarm && bmsData.packTemp > TEMP_DISCHARGE_RECOVERY_LOW) {
            bmsData.underTempDischargeAlarm = false;
        }
    }
    
    // ===== 4. NHIỆT ĐỘ KHI SẠC - WARNING & CRITICAL =====
    bmsData.overTempChargeWarning = false;
    bmsData.overTempChargeAlarm = false;
    bmsData.underTempChargeWarning = false;
    bmsData.underTempChargeAlarm = false;
    
    if (bmsData.isCharging) {
        // Nhiệt độ cao
        if (bmsData.packTemp > TEMP_CHARGE_WARNING_HIGH) {
            bmsData.overTempChargeWarning = true;
        }
        if (bmsData.packTemp >= TEMP_CHARGE_CRITICAL_HIGH) {
            bmsData.overTempChargeAlarm = true;
        }
        // Recovery
        if (bmsData.overTempChargeAlarm && bmsData.packTemp < TEMP_CHARGE_RECOVERY_HIGH) {
            bmsData.overTempChargeAlarm = false;
        }
        
        // Nhiệt độ thấp
        if (bmsData.packTemp < TEMP_CHARGE_WARNING_LOW) {
            bmsData.underTempChargeWarning = true;
        }
        if (bmsData.packTemp <= TEMP_CHARGE_CRITICAL_LOW) {
            bmsData.underTempChargeAlarm = true;
        }
        // Recovery
        if (bmsData.underTempChargeAlarm && bmsData.packTemp > TEMP_CHARGE_RECOVERY_LOW) {
            bmsData.underTempChargeAlarm = false;
        }
    }
}

// Cập nhật charging status
void updateChargingStatus() {
    if (bmsData.current > 0.2) {
        bmsData.isCharging = true;
        bmsData.isDischarging = false;
    } else if (bmsData.current < -0.2) {
        bmsData.isCharging = false;
        bmsData.isDischarging = true;
    } else {
        bmsData.isCharging = false;
        bmsData.isDischarging = false;
    }
}

// Cập nhật BMS data từ sensors và SOC/SOH
void updateBMSData(float cell1, float cell2, float cell3, float cell4, 
                   float current, float temp) {
    // Cập nhật cell voltages
    bmsData.cellVoltages[0] = cell1;
    bmsData.cellVoltages[1] = cell2;
    bmsData.cellVoltages[2] = cell3;
    bmsData.cellVoltages[3] = cell4;
    
    // Cập nhật pack voltage
    bmsData.packVoltage = cell1 + cell2 + cell3 + cell4;
    
    // Tính average cell voltage
    bmsData.avgCellVoltage = bmsData.packVoltage / NUM_CELLS;
    
    // Cập nhật current và temp
    bmsData.current = current;
    bmsData.packTemp = temp;
    
    // ======== LẤY SOC VÀ SOH TỪ ESTIMATORS ========
    if (socInitialized) {
        bmsData.soc = soc.getSOC();
    } else {
        bmsData.soc = 50.0; // Default nếu chưa init
    }
    
    if (sohInitialized) {
        bmsData.soh = soh.getSOH();
        bmsData.totalCycles = soh.getTotalCycles();
        bmsData.remainingCapacity = soh.getCurrentCapacity();
    } else {
        bmsData.soh = 100.0;
        bmsData.totalCycles = 0.0;
        bmsData.remainingCapacity = BATTERY_CAPACITY;
    }
    
    // Kiểm tra các điều kiện (phải theo thứ tự này)
    updateChargingStatus();  // Xác định trạng thái sạc/xả trước
    checkProtection();       // Sau đó mới check protection
    checkBalancing();
    
    bmsData.systemActive = true;
    bmsData.lastUpdateTime = millis();
}

// Tạo JSON response
String getBMSJson() {
    StaticJsonDocument<2048> doc;
    
    // ============ MEASUREMENT ============
    JsonObject measurement = doc.createNestedObject("measurement");
    
    JsonArray cells = measurement.createNestedArray("cellVoltages");
    for (int i = 0; i < NUM_CELLS; i++) {
        JsonObject cell = cells.createNestedObject();
        cell["cell"] = i + 1;
        cell["voltage"] = String(bmsData.cellVoltages[i], 3);
    }
    
    measurement["packVoltage"] = String(bmsData.packVoltage, 2);
    measurement["avgCellVoltage"] = String(bmsData.avgCellVoltage, 3);
    measurement["current"] = String(bmsData.current, 2);
    measurement["packTemperature"] = String(bmsData.packTemp, 1);
    
    // ============ CALCULATION (SOC/SOH) ============
    JsonObject calculation = doc.createNestedObject("calculation");
    calculation["soc"] = String(bmsData.soc, 1);
    calculation["soh"] = String(bmsData.soh, 1);
    calculation["remainingCapacity"] = String(bmsData.remainingCapacity, 3);
    calculation["totalCycles"] = String(bmsData.totalCycles, 1);
    calculation["remainingCycles"] = String(soh.getRemainingCycles(), 0);
    
    // ============ STATUS ============
    JsonObject status = doc.createNestedObject("status");
    
    if (bmsData.isCharging) {
        status["charging"] = "charging";
    } else if (bmsData.isDischarging) {
        status["charging"] = "discharging";
    } else {
        status["charging"] = "idle";
    }
    
    JsonObject balancing = status.createNestedObject("balancing");
    balancing["active"] = bmsData.balancingActive;
    
    JsonArray balancingCellsArray = balancing.createNestedArray("cells");
    if (bmsData.balancingActive) {
        for (int i = 0; i < NUM_CELLS; i++) {
            if (bmsData.balancingCells[i]) {
                balancingCellsArray.add(i + 1);
            }
        }
    }
    
    // ============ PROTECTION ============
    JsonObject protection = doc.createNestedObject("protection");
    protection["overVoltage"] = statusToString(bmsData.overVoltageAlarm);
    protection["underVoltage"] = statusToString(bmsData.underVoltageAlarm);
    protection["overCurrentCharge"] = statusToString(bmsData.overCurrentChargeAlarm);
    protection["overCurrentDischarge"] = statusToString(bmsData.overCurrentDischargeAlarm);
    protection["overTempCharge"] = statusToString(bmsData.overTempChargeAlarm);
    protection["overTempDischarge"] = statusToString(bmsData.overTempDischargeAlarm);
    protection["underTempCharge"] = statusToString(bmsData.underTempChargeAlarm);
    protection["underTempDischarge"] = statusToString(bmsData.underTempDischargeAlarm);
    
    // ============ ALERTS ============
    JsonArray alerts = doc.createNestedArray("alerts");
    
    // ===== CRITICAL ALERTS (🚨) =====
    if (bmsData.overVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt sạc: Điện áp quá cao!";
    }
    
    if (bmsData.underVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt xả: Điện áp quá thấp!";
    }
    
    if (bmsData.overCurrentChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt sạc: Quá dòng sạc!";
    }
    
    if (bmsData.overCurrentDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt xả: Quá dòng xả!";
    }
    
    if (bmsData.overTempChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt sạc: Nhiệt độ quá cao!";
    }
    
    if (bmsData.underTempChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt sạc: Nhiệt độ quá thấp!";
    }
    
    if (bmsData.overTempDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt xả: Nhiệt độ quá cao!";
    }
    
    if (bmsData.underTempDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["icon"] = "🚨";
        alert["message"] = "Ngắt xả: Nhiệt độ quá thấp!";
    }
    
    // ===== WARNING ALERTS (⚠️) =====
    if (bmsData.overVoltageWarning && !bmsData.overVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Điện áp cell đang cao";
    }
    
    if (bmsData.underVoltageWarning && !bmsData.underVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Điện áp cell đang thấp";
    }
    
    if (bmsData.overCurrentChargeWarning && !bmsData.overCurrentChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Dòng sạc đang cao";
    }
    
    if (bmsData.overCurrentDischargeWarning && !bmsData.overCurrentDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Dòng xả đang cao";
    }
    
    if (bmsData.overTempChargeWarning && !bmsData.overTempChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Cảnh báo: Nhiệt độ sạc đang cao";
    }
    
    if (bmsData.underTempChargeWarning && !bmsData.underTempChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Cảnh báo: Nhiệt độ sạc đang thấp";
    }
    
    if (bmsData.overTempDischargeWarning && !bmsData.overTempDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Cảnh báo: Nhiệt độ xả đang cao";
    }
    
    if (bmsData.underTempDischargeWarning && !bmsData.underTempDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["icon"] = "⚠️";
        alert["message"] = "Cảnh báo: Nhiệt độ xả đang thấp";
    }
    
    // ===== BALANCING WARNING =====
    if (bmsData.balancingActive) {
        float maxV = bmsData.cellVoltages[0];
        float minV = bmsData.cellVoltages[0];
        for (int i = 1; i < NUM_CELLS; i++) {
            if (bmsData.cellVoltages[i] > maxV) maxV = bmsData.cellVoltages[i];
            if (bmsData.cellVoltages[i] < minV) minV = bmsData.cellVoltages[i];
        }
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

void initBMSData() {
    bmsData.packVoltage = 0;
    bmsData.avgCellVoltage = 0;
    bmsData.current = 0;
    bmsData.packTemp = 25.0;
    bmsData.soc = 100.0;
    bmsData.soh = 100.0;
    bmsData.totalCycles = 0.0;
    bmsData.remainingCapacity = BATTERY_CAPACITY;
    
    for (int i = 0; i < NUM_CELLS; i++) {
        bmsData.cellVoltages[i] = 0;
        bmsData.balancingCells[i] = false;
    }
    
    // Critical alarms
    bmsData.overVoltageAlarm = false;
    bmsData.underVoltageAlarm = false;
    bmsData.overCurrentChargeAlarm = false;
    bmsData.overCurrentDischargeAlarm = false;
    bmsData.overTempChargeAlarm = false;
    bmsData.overTempDischargeAlarm = false;
    bmsData.underTempChargeAlarm = false;
    bmsData.underTempDischargeAlarm = false;
    
    // Warnings
    bmsData.overVoltageWarning = false;
    bmsData.underVoltageWarning = false;
    bmsData.overCurrentChargeWarning = false;
    bmsData.overCurrentDischargeWarning = false;
    bmsData.overTempChargeWarning = false;
    bmsData.overTempDischargeWarning = false;
    bmsData.underTempChargeWarning = false;
    bmsData.underTempDischargeWarning = false;
    
    bmsData.balancingActive = false;
    bmsData.isCharging = false;
    bmsData.isDischarging = false;
    bmsData.systemActive = false;
    bmsData.lastUpdateTime = 0;
    bmsData.idleStartTime = 0;
}

#endif