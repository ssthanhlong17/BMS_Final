#include "bms_data.h"

// ==================== GLOBAL INSTANCE ====================
BMSData bmsData;

// ==================== HELPER ====================
const char* statusToString(bool alarm) {
    return alarm ? "alarm" : "normal";
}

// ==================== STATE ====================
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

// ==================== DISPLAY PROTECTION ====================
void checkProtectionStatus() {
    bmsData.underVoltageWarning = false;
    bmsData.underVoltageAlarm = false;
    bmsData.overVoltageWarning = false;
    bmsData.overVoltageAlarm = false;

    for (int i = 0; i < NUM_CELLS; i++) {
        float v = bmsData.cellVoltages[i];
        if (v < CELL_UV_WARNING)  bmsData.underVoltageWarning = true;
        if (v < CELL_UV_CRITICAL) bmsData.underVoltageAlarm = true;
        if (v > CELL_OV_WARNING)  bmsData.overVoltageWarning = true;
        if (v > CELL_OV_CRITICAL) bmsData.overVoltageAlarm = true;
    }

    bmsData.overCurrentChargeWarning = false;
    bmsData.overCurrentChargeAlarm = false;
    bmsData.overCurrentDischargeWarning = false;
    bmsData.overCurrentDischargeAlarm = false;

    if (bmsData.isCharging) {
        if (bmsData.current > CURRENT_CHARGE_WARNING)
            bmsData.overCurrentChargeWarning = true;
        if (bmsData.current > CURRENT_CHARGE_CRITICAL)
            bmsData.overCurrentChargeAlarm = true;
    }

    if (bmsData.isDischarging) {
        if (bmsData.current < CURRENT_DISCHARGE_WARNING)
            bmsData.overCurrentDischargeWarning = true;
        if (bmsData.current < CURRENT_DISCHARGE_CRITICAL)
            bmsData.overCurrentDischargeAlarm = true;
    }

    bmsData.overTempChargeWarning = false;
    bmsData.overTempChargeAlarm = false;
    bmsData.overTempDischargeWarning = false;
    bmsData.overTempDischargeAlarm = false;

    if (bmsData.isCharging) {
        if (bmsData.packTemp > TEMP_CHARGE_WARNING_HIGH)
            bmsData.overTempChargeWarning = true;
        if (bmsData.packTemp >= TEMP_CHARGE_CRITICAL_HIGH)
            bmsData.overTempChargeAlarm = true;
    }

    if (bmsData.isDischarging) {
        if (bmsData.packTemp > TEMP_DISCHARGE_WARNING_HIGH)
            bmsData.overTempDischargeWarning = true;
        if (bmsData.packTemp >= TEMP_DISCHARGE_CRITICAL_HIGH)
            bmsData.overTempDischargeAlarm = true;
    }
}

// ==================== MAIN UPDATE ====================
void updateAllBMSData() {
    sensors.readAllSensors();

    for (int i = 0; i < NUM_CELLS; i++) {
        bmsData.cellVoltages[i] = sensors.getCellVoltage(i + 1);
    }

    bmsData.packVoltage = sensors.getPackVoltage();
    bmsData.avgCellVoltage = bmsData.packVoltage / NUM_CELLS;
    bmsData.current = sensors.getCurrent();
    bmsData.packTemp = sensors.getTemperature();

    if (!socInitialized) {
        soc.initializeFromVoltage(bmsData.packVoltage);
        socInitialized = true;
    }

    protection.update(
        bmsData.cellVoltages[0],
        bmsData.cellVoltages[1],
        bmsData.cellVoltages[2],
        bmsData.cellVoltages[3],
        bmsData.current,
        bmsData.packTemp
    );

    bmsData.chargeMosfetEnabled = protection.getChargeMosfetState();
    bmsData.dischargeMosfetEnabled = protection.getDischargeMosfetState();

    balancing.update(
        bmsData.cellVoltages[0],
        bmsData.cellVoltages[1],
        bmsData.cellVoltages[2],
        bmsData.cellVoltages[3],
        bmsData.current
    );

    bmsData.balancingActive = balancing.isActive();
    bmsData.balancingCell = balancing.getBalancingCell();
    balancing.getBalancingStatus(bmsData.balancingCells);

    bmsData.soc = socInitialized ? soc.getSOC() : 50.0;

    if (sohInitialized) {
        bmsData.soh = soh.getSOH();
        bmsData.totalCycles = soh.getTotalCycles();
        bmsData.remainingCapacity = soh.getCurrentCapacity();
        bmsData.remainingCycles = soh.getRemainingCycles();
    }

    updateChargingStatus();
    checkProtectionStatus();

    bmsData.systemActive = true;
    bmsData.lastUpdateTime = millis();
}

// ==================== SOC / SOH ====================
void updateSOC() {
    if (!socInitialized) return;
    soc.update(bmsData.current, bmsData.packTemp);
    soc.recalibrate(bmsData.packVoltage, bmsData.current);
    bmsData.soc = soc.getSOC();
}

void updateSOH() {
    if (!sohInitialized || !socInitialized) return;
    soh.update(bmsData.soc, bmsData.packTemp);
    bmsData.soh = soh.getSOH();
    bmsData.totalCycles = soh.getTotalCycles();
    bmsData.remainingCapacity = soh.getCurrentCapacity();
    bmsData.remainingCycles = soh.getRemainingCycles();
}

// ==================== JSON API ====================
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
    JsonObject protectionObj = doc.createNestedObject("protection");
    protectionObj["overVoltage"] = bmsData.overVoltageAlarm ? "alarm" : "normal";
    protectionObj["underVoltage"] = bmsData.underVoltageAlarm ? "alarm" : "normal";
    protectionObj["overCurrentCharge"] = bmsData.overCurrentChargeAlarm ? "alarm" : "normal";
    protectionObj["overCurrentDischarge"] = bmsData.overCurrentDischargeAlarm ? "alarm" : "normal";
    protectionObj["overTempCharge"] = bmsData.overTempChargeAlarm ? "alarm" : "normal";
    protectionObj["overTempDischarge"] = bmsData.overTempDischargeAlarm ? "alarm" : "normal";
   
    // ============ ALERTS ============
    JsonArray alerts = doc.createNestedArray("alerts");
   
    if (bmsData.overVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "Ngắt sạc: Điện áp quá cao!";
    }
   
    if (bmsData.underVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "Ngắt xả: Điện áp quá thấp!";
    }
   
    if (bmsData.overCurrentChargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "Ngắt sạc: Quá dòng sạc!";
    }
   
    if (bmsData.overCurrentDischargeAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "critical";
        alert["message"] = "Ngắt xả: Quá dòng xả!";
    }
   
    if (bmsData.overVoltageWarning && !bmsData.overVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["message"] = "Điện áp cell đang cao";
    }
   
    if (bmsData.underVoltageWarning && !bmsData.underVoltageAlarm) {
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "warning";
        alert["message"] = "Điện áp cell đang thấp";
    }
   
    if (bmsData.balancingActive) {
        float maxV = bmsData.cellVoltages[0];
        float minV = bmsData.cellVoltages[0];
        for (int i = 1; i < NUM_CELLS; i++) {
            if (bmsData.cellVoltages[i] > maxV) maxV = bmsData.cellVoltages[i];
            if (bmsData.cellVoltages[i] < minV) minV = bmsData.cellVoltages[i];
        }
       
        JsonObject alert = alerts.createNestedObject();
        alert["severity"] = "info";
        alert["message"] = String("Đang cân bằng Cell ") + String(bmsData.balancingCell) +
                          " (Δ" + String(maxV - minV, 3) + "V)";
    }
   
    String output;
    serializeJson(doc, output);
    return output;
}
