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

// ==================== NGƯỠNG BẢO VỆ (DISPLAY) ====================
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
    float cellVoltages[NUM_CELLS];
    float packVoltage;
    float current;
    float packTemp;
    float soc;
    float soh;
    float avgCellVoltage;

    // Alarm
    bool overVoltageAlarm;
    bool underVoltageAlarm;
    bool overCurrentChargeAlarm;
    bool overCurrentDischargeAlarm;
    bool overTempChargeAlarm;
    bool overTempDischargeAlarm;
    bool underTempChargeAlarm;
    bool underTempDischargeAlarm;

    // Warning
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
    uint8_t balancingCell;

    // State
    bool isCharging;
    bool isDischarging;
    bool systemActive;

    // MOSFET
    bool chargeMosfetEnabled;
    bool dischargeMosfetEnabled;

    // Runtime
    unsigned long lastUpdateTime;
    float totalCycles;
    float remainingCapacity;
    float remainingCycles;
};

// ==================== GLOBAL DATA ====================
extern BMSData bmsData;

// ==================== EXTERNAL OBJECTS ====================
extern BMSSensors sensors;
extern BMSProtection protection;
extern BMSBalancing balancing;
extern BMSDwin dwin;
extern SOCEstimator soc;
extern SOHEstimator soh;
extern bool socInitialized;
extern bool sohInitialized;

// ==================== FUNCTIONS ====================
const char* statusToString(bool alarm);

void updateChargingStatus();
void checkProtectionStatus();
void updateAllBMSData();

void updateSOC();
void updateSOH();
void updateDWINDisplay();

String getBMSJson();
void initBMSData();

#endif
