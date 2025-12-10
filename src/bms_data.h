#ifndef BMS_DATA_H
#define BMS_DATA_H

#include <ArduinoJson.h>
#include "soc_estimator.h"
#include "soh_estimator.h"

const int NUM_CELLS = 4;

// Ngưỡng bảo vệ (điều chỉnh cho LiFePO4)
#define CELL_OV_THRESHOLD 3.65   // Over voltage (LiFePO4)
#define CELL_UV_THRESHOLD 2.50   // Under voltage (LiFePO4)
#define PACK_OC_THRESHOLD 5.0    // Over current (A)
#define PACK_OT_THRESHOLD 50.0   // Over temperature (°C)
#define CELL_BALANCE_DIFF 0.05   // 50mV difference to trigger balancing

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
    
    // Protection status
    bool overVoltageAlarm;
    bool underVoltageAlarm;
    bool overCurrentAlarm;
    bool overTempAlarm;
    bool shortCircuitAlarm;
    
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

// Kiểm tra balancing cần thiết
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
    
    if (diff > CELL_BALANCE_DIFF) {
        bmsData.balancingActive = true;
        for (int i = 0; i < NUM_CELLS; i++) {
            if (bmsData.cellVoltages[i] >= (maxV - 0.01)) {
                bmsData.balancingCells[i] = true;
            } else {
                bmsData.balancingCells[i] = false;
            }
        }
    } else {
        bmsData.balancingActive = false;
        for (int i = 0; i < NUM_CELLS; i++) {
            bmsData.balancingCells[i] = false;
        }
    }
}

// Kiểm tra protection
void checkProtection() {
    // Over/Under Voltage
    bmsData.overVoltageAlarm = false;
    bmsData.underVoltageAlarm = false;
    
    for (int i = 0; i < NUM_CELLS; i++) {
        if (bmsData.cellVoltages[i] > CELL_OV_THRESHOLD) {
            bmsData.overVoltageAlarm = true;
        }
        if (bmsData.cellVoltages[i] < CELL_UV_THRESHOLD) {
            bmsData.underVoltageAlarm = true;
        }
    }
    
    // Over Current
    bmsData.overCurrentAlarm = (abs(bmsData.current) > PACK_OC_THRESHOLD);
    
    // Over Temperature
    bmsData.overTempAlarm = (bmsData.packTemp > PACK_OT_THRESHOLD);
    
    // Short Circuit (dòng cực lớn)
    bmsData.shortCircuitAlarm = (abs(bmsData.current) > 10.0);
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
    
    // Kiểm tra các điều kiện
    checkProtection();
    checkBalancing();
    updateChargingStatus();
    
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
    protection["overCurrent"] = statusToString(bmsData.overCurrentAlarm);
    protection["overTemperature"] = statusToString(bmsData.overTempAlarm);
    protection["shortCircuit"] = statusToString(bmsData.shortCircuitAlarm);
    
    // ============ ALERTS ============
    JsonArray alerts = doc.createNestedArray("alerts");
    
    if (bmsData.overVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "⚠️ Điện áp quá cao!";
    }

    if (bmsData.underVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "⚠️ Điện áp quá thấp!";
    }

    if (bmsData.overCurrentAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "⚠️ Dòng điện quá tải!";
    }

    if (bmsData.overTempAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "⚠️ Nhiệt độ quá cao!";
    }

    if (bmsData.shortCircuitAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "⚠️ Ngắn mạch!";
    }

    if (bmsData.balancingActive) {
        float maxV = bmsData.cellVoltages[0];
        float minV = bmsData.cellVoltages[0];
        for (int i = 1; i < NUM_CELLS; i++) {
            if (bmsData.cellVoltages[i] > maxV) maxV = bmsData.cellVoltages[i];
            if (bmsData.cellVoltages[i] < minV) minV = bmsData.cellVoltages[i];
        }
        if ((maxV - minV) > 0.05) {
            JsonObject alert = alerts.createNestedObject();
            alert["severity"] = "warning";
            alert["message"] = "⚠️ Chênh lệch điện áp giữa các cell quá lớn";
        }
    }

    // Cảnh báo SOH
    if (bmsData.soh < 90.0) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        char msg[64];
        sprintf(msg, "⚠️ SOH giảm: %.1f%%", bmsData.soh);
        alert["message"] = msg;
    }

    if (bmsData.soh < 80.0) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "🚨 Pin gần hết tuổi thọ (SOH < 80%)";
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
    
    bmsData.overVoltageAlarm = false;
    bmsData.underVoltageAlarm = false;
    bmsData.overCurrentAlarm = false;
    bmsData.overTempAlarm = false;
    bmsData.shortCircuitAlarm = false;
    bmsData.balancingActive = false;
    bmsData.isCharging = false;
    bmsData.isDischarging = false;
    bmsData.systemActive = false;
    bmsData.lastUpdateTime = 0;
    bmsData.idleStartTime = 0;
}

#endif