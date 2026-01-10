#ifndef BMS_DATA_H
#define BMS_DATA_H

#include <ArduinoJson.h>
#include "soc_estimator.h"
#include "soh_estimator.h"
#include "bms_sensors.h"
#include "bms_protection.h"
#include "bms_balancing.h"
#include "bms_dwin.h"

const int NUM_CELLS = 4;

// ==================== NGƯỠNG BẢO VỆ (CHỈ DÙNG CHO DISPLAY) ====================
#define CELL_UV_WARNING 3.0
#define CELL_UV_CRITICAL 2.5
#define CELL_OV_WARNING 3.45
#define CELL_OV_CRITICAL 3.65

#define CURRENT_DISCHARGE_WARNING -4.0
#define CURRENT_DISCHARGE_CRITICAL -6.0
#define CURRENT_CHARGE_WARNING 1.4
#define CURRENT_CHARGE_CRITICAL 2.0

#define TEMP_DISCHARGE_WARNING_HIGH 55.0
#define TEMP_DISCHARGE_CRITICAL_HIGH 60.0
#define TEMP_CHARGE_WARNING_HIGH 40.0
#define TEMP_CHARGE_CRITICAL_HIGH 45.0

#define BATTERY_CAPACITY 6.0

// ==================== BMS DATA STRUCT ====================
struct BMSData {
    // Measurements
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
    uint8_t balancingCell;  // Cell đang được balance (1-4)
    
    // Charging status
    bool isCharging;
    bool isDischarging;
    bool systemActive;
    
    // MOSFETs
    bool chargeMosfetEnabled;
    bool dischargeMosfetEnabled;
    
    // Runtime data
    unsigned long lastUpdateTime;
    float totalCycles;
    float remainingCapacity;
    float remainingCycles;
};

BMSData bmsData;

// ==================== EXTERNAL OBJECTS ====================
// Được khởi tạo trong main.cpp
extern BMSSensors sensors;
extern BMSProtection protection;
extern BMSBalancing balancing;
extern BMSDwin dwin;
extern SOCEstimator soc;
extern SOHEstimator soh;
extern bool socInitialized;
extern bool sohInitialized;

// ==================== HELPER FUNCTIONS ====================

const char* statusToString(bool alarm) {
    return alarm ? "alarm" : "normal";
}

void updateChargingStatus() {
    if (bmsData.current > 0.1) {
        bmsData.isCharging = true;
        bmsData.isDischarging = false;
    } else if (bmsData.current < -0.1) {
        bmsData.isCharging = false;
        bmsData.isDischarging = true;
    } else {
        bmsData.isCharging = false;
        bmsData.isDischarging = false;
    }
}

void checkProtectionStatus() {
    // ===== ĐIỆN ÁP =====
    bmsData.underVoltageWarning = false;
    bmsData.underVoltageAlarm = false;
    bmsData.overVoltageWarning = false;
    bmsData.overVoltageAlarm = false;
    
    for (int i = 0; i < NUM_CELLS; i++) {
        if (bmsData.cellVoltages[i] < CELL_UV_WARNING) {
            bmsData.underVoltageWarning = true;
        }
        if (bmsData.cellVoltages[i] < CELL_UV_CRITICAL) {
            bmsData.underVoltageAlarm = true;
        }
        if (bmsData.cellVoltages[i] > CELL_OV_WARNING) {
            bmsData.overVoltageWarning = true;
        }
        if (bmsData.cellVoltages[i] > CELL_OV_CRITICAL) {
            bmsData.overVoltageAlarm = true;
        }
    }
    
    // ===== DÒNG ĐIỆN =====
    bmsData.overCurrentDischargeWarning = false;
    bmsData.overCurrentDischargeAlarm = false;
    bmsData.overCurrentChargeWarning = false;
    bmsData.overCurrentChargeAlarm = false;
    
    if (bmsData.isDischarging) {
        if (bmsData.current < CURRENT_DISCHARGE_WARNING) {
            bmsData.overCurrentDischargeWarning = true;
        }
        if (bmsData.current < CURRENT_DISCHARGE_CRITICAL) {
            bmsData.overCurrentDischargeAlarm = true;
        }
    }
    
    if (bmsData.isCharging) {
        if (bmsData.current > CURRENT_CHARGE_WARNING) {
            bmsData.overCurrentChargeWarning = true;
        }
        if (bmsData.current > CURRENT_CHARGE_CRITICAL) {
            bmsData.overCurrentChargeAlarm = true;
        }
    }
    
    // ===== NHIỆT ĐỘ =====
    bmsData.overTempDischargeWarning = false;
    bmsData.overTempDischargeAlarm = false;
    bmsData.overTempChargeWarning = false;
    bmsData.overTempChargeAlarm = false;
    bmsData.underTempDischargeWarning = false;
    bmsData.underTempDischargeAlarm = false;
    bmsData.underTempChargeWarning = false;
    bmsData.underTempChargeAlarm = false;
    
    if (bmsData.isDischarging) {
        if (bmsData.packTemp > TEMP_DISCHARGE_WARNING_HIGH) {
            bmsData.overTempDischargeWarning = true;
        }
        if (bmsData.packTemp >= TEMP_DISCHARGE_CRITICAL_HIGH) {
            bmsData.overTempDischargeAlarm = true;
        }
    }
    
    if (bmsData.isCharging) {
        if (bmsData.packTemp > TEMP_CHARGE_WARNING_HIGH) {
            bmsData.overTempChargeWarning = true;
        }
        if (bmsData.packTemp >= TEMP_CHARGE_CRITICAL_HIGH) {
            bmsData.overTempChargeAlarm = true;
        }
    }
}

// ==================== 🎯 HÀM TRUNG TÂM - THU THẬP TẤT CẢ DỮ LIỆU ====================
void updateAllBMSData() {
    // ===== 1. ĐỌC SENSORS =====
    sensors.readAllSensors();
    
    // Thu thập measurements
    bmsData.cellVoltages[0] = sensors.getCellVoltage(1);
    bmsData.cellVoltages[1] = sensors.getCellVoltage(2);
    bmsData.cellVoltages[2] = sensors.getCellVoltage(3);
    bmsData.cellVoltages[3] = sensors.getCellVoltage(4);
    bmsData.packVoltage = sensors.getPackVoltage();
    bmsData.avgCellVoltage = bmsData.packVoltage / NUM_CELLS;
    bmsData.current = sensors.getCurrent();
    bmsData.packTemp = sensors.getTemperature();
    
    // ===== 2. KHỞI TẠO SOC LẦN ĐẦU =====
    if (!socInitialized) {
        soc.initializeFromVoltage(bmsData.packVoltage);
        socInitialized = true;
        Serial.printf("🔋 SOC initialized: %.1f%%\n", soc.getSOC());
    }
    
    // ===== 3. CẬP NHẬT PROTECTION (XỬ LÝ MOSFET) =====
    protection.update(
        bmsData.cellVoltages[0],
        bmsData.cellVoltages[1],
        bmsData.cellVoltages[2],
        bmsData.cellVoltages[3],
        bmsData.current,
        bmsData.packTemp
    );
    
    // Lấy trạng thái MOSFETs
    bmsData.chargeMosfetEnabled = protection.getChargeMosfetState();
    bmsData.dischargeMosfetEnabled = protection.getDischargeMosfetState();
    
    // ===== 4. CẬP NHẬT BALANCING (ĐIỀU KHIỂN PINS) =====
    balancing.update(
        bmsData.cellVoltages[0],
        bmsData.cellVoltages[1],
        bmsData.cellVoltages[2],
        bmsData.cellVoltages[3],
        bmsData.current
    );
    
    // Lấy trạng thái balancing
    bmsData.balancingActive = balancing.isActive();
    bmsData.balancingCell = balancing.getBalancingCell();
    balancing.getBalancingStatus(bmsData.balancingCells);
    
    // ===== 5. LẤY SOC & SOH =====
    if (socInitialized) {
        bmsData.soc = soc.getSOC();
    } else {
        bmsData.soc = 50.0;
    }
    
    if (sohInitialized) {
        bmsData.soh = soh.getSOH();
        bmsData.totalCycles = soh.getTotalCycles();
        bmsData.remainingCapacity = soh.getCurrentCapacity();
        bmsData.remainingCycles = soh.getRemainingCycles();
    } else {
        bmsData.soh = 100.0;
        bmsData.totalCycles = 0.0;
        bmsData.remainingCapacity = BATTERY_CAPACITY;
        bmsData.remainingCycles = 2000.0;
    }
    
    // ===== 6. XÁC ĐỊNH TRẠNG THÁI SẠC/XẢ =====
    updateChargingStatus();
    
    // ===== 7. KIỂM TRA PROTECTION STATUS (CHO DISPLAY) =====
    checkProtectionStatus();
    
    // ===== 8. TIMESTAMP =====
    bmsData.systemActive = true;
    bmsData.lastUpdateTime = millis();
}

// ==================== CẬP NHẬT CHỈ SOC (GỌI MỖI 1S) ====================
void updateSOC() {
    if (!socInitialized) return;
    
    soc.update(bmsData.current, bmsData.packTemp);
    soc.recalibrate(bmsData.packVoltage, bmsData.current);
    bmsData.soc = soc.getSOC();
}

// ==================== CẬP NHẬT CHỈ SOH (GỌI MỖI 10S) ====================
void updateSOH() {
    if (!sohInitialized || !socInitialized) return;
    
    soh.update(bmsData.soc, bmsData.packTemp);
    bmsData.soh = soh.getSOH();
    bmsData.totalCycles = soh.getTotalCycles();
    bmsData.remainingCapacity = soh.getCurrentCapacity();
    bmsData.remainingCycles = soh.getRemainingCycles();
}

// ==================== CẬP NHẬT DWIN DISPLAY (GỌI MỖI 1S) ====================
void updateDWINDisplay() {
    dwin.updateBasicData(
        bmsData.cellVoltages[0],
        bmsData.cellVoltages[1],
        bmsData.cellVoltages[2],
        bmsData.cellVoltages[3],
        bmsData.packVoltage,
        bmsData.current,
        bmsData.packTemp
    );
    
    dwin.updateAllWarnings(
        bmsData.overVoltageWarning,
        bmsData.overVoltageAlarm,
        bmsData.overCurrentChargeWarning,
        bmsData.overCurrentChargeAlarm,
        bmsData.overTempChargeWarning,
        bmsData.overTempChargeAlarm,
        bmsData.underVoltageWarning,
        bmsData.underVoltageAlarm,
        bmsData.overCurrentDischargeWarning,
        bmsData.overCurrentDischargeAlarm,
        bmsData.overTempDischargeWarning,
        bmsData.overTempDischargeAlarm
    );
}

// ==================== TẠO JSON (KHÔNG THAY ĐỔI) ====================
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
    
    // ============ CALCULATION ============
    JsonObject calculation = doc.createNestedObject("calculation");
    calculation["soc"] = String(bmsData.soc, 1);
    calculation["soh"] = String(bmsData.soh, 1);
    calculation["remainingCapacity"] = String(bmsData.remainingCapacity, 3);
    calculation["totalCycles"] = String(bmsData.totalCycles, 1);
    calculation["remainingCycles"] = String(bmsData.remainingCycles, 0);
    
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
    
    // Critical alarms
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
    
    // Warnings
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
    
    // Balancing info
    if (bmsData.balancingActive) {
        float maxV = bmsData.cellVoltages[0];
        float minV = bmsData.cellVoltages[0];
        for (int i = 1; i < NUM_CELLS; i++) {
            if (bmsData.cellVoltages[i] > maxV) maxV = bmsData.cellVoltages[i];
            if (bmsData.cellVoltages[i] < minV) minV = bmsData.cellVoltages[i];
        }
        
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "info";
        alert["icon"] = "⚖️";
        alert["message"] = String("Đang cân bằng Cell ") + String(bmsData.balancingCell) + 
                          " (Δ" + String(maxV - minV, 3) + "V)";
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ==================== INIT ====================
void initBMSData() {
    bmsData.packVoltage = 0;
    bmsData.avgCellVoltage = 0;
    bmsData.current = 0;
    bmsData.packTemp = 25.0;
    bmsData.soc = 100.0;
    bmsData.soh = 100.0;
    bmsData.totalCycles = 0.0;
    bmsData.remainingCapacity = BATTERY_CAPACITY;
    bmsData.remainingCycles = 2000.0;
    
    for (int i = 0; i < NUM_CELLS; i++) {
        bmsData.cellVoltages[i] = 0;
        bmsData.balancingCells[i] = false;
    }
    
    bmsData.balancingCell = 0;
    
    // Alarms & Warnings
    bmsData.overVoltageAlarm = false;
    bmsData.underVoltageAlarm = false;
    bmsData.overCurrentChargeAlarm = false;
    bmsData.overCurrentDischargeAlarm = false;
    bmsData.overTempChargeAlarm = false;
    bmsData.overTempDischargeAlarm = false;
    bmsData.underTempChargeAlarm = false;
    bmsData.underTempDischargeAlarm = false;
    
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
    bmsData.chargeMosfetEnabled = true;
    bmsData.dischargeMosfetEnabled = true;
    bmsData.lastUpdateTime = 0;
}

#endif