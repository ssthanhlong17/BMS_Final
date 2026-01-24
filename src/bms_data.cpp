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
