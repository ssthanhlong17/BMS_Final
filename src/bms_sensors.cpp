#include "bms_sensors.h"

BMSSensors::BMSSensors() {
    for (int i = 0; i < 4; i++) {
        cellVoltages[i] = 0.0f;
    }
    tap1 = tap2 = tap3 = tap4 = 0.0f;
    packVoltage = 0.0f;
    current = 0.0f;
    temperature = 25.0f;
    lastReadTime = 0;
}

void BMSSensors::begin() {
    
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_T1, ADC_11db);
    analogSetPinAttenuation(PIN_T2, ADC_11db);
    analogSetPinAttenuation(PIN_T3, ADC_11db);
    analogSetPinAttenuation(PIN_T4, ADC_11db);
    analogSetPinAttenuation(PIN_I, ADC_11db);
    analogSetPinAttenuation(PIN_TEMP, ADC_11db);
    
    delay(100);
    readAllSensors();
    
    Serial.println("BMS Sensors initialized");
    Serial.printf("Initial Pack Voltage: %.2fV\n", packVoltage);
    Serial.printf("Initial Temperature: %.1f°C\n", temperature);
}

uint16_t BMSSensors::readMilliVoltsMedian(int pin, int n) {
    static uint16_t buf[64];
    if (n > 64) n = 64;
    
    for (int i = 0; i < n; i++) {
        buf[i] = analogReadMilliVolts(pin);
        delayMicroseconds(200);
    }
    
    for (int i = 1; i < n; i++) {
        uint16_t key = buf[i];
        int j = i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }
    
    return buf[n / 2];
}

void BMSSensors::readTaps() {
    float adc1 = readMilliVoltsMedian(PIN_T1) / 1000.0f;
    float adc2 = readMilliVoltsMedian(PIN_T2) / 1000.0f;
    float adc3 = readMilliVoltsMedian(PIN_T3) / 1000.0f;
    float adc4 = readMilliVoltsMedian(PIN_T4) / 1000.0f;
    
    tap1 = (adc1 - OFF1) * DIV1;
    tap2 = (adc2 - OFF2) * DIV2;
    tap3 = (adc3 - OFF3) * DIV3;
    tap4 = (adc4 - OFF4) * DIV4;
}

void BMSSensors::calculateCellVoltages() {
    cellVoltages[0] = tap1 - tap2;
    cellVoltages[1] = tap2 - tap3;
    cellVoltages[2] = tap3 - tap4;
    cellVoltages[3] = tap4;
    
    packVoltage = cellVoltages[0] + cellVoltages[1] + 
                  cellVoltages[2] + cellVoltages[3];
}

void BMSSensors::readCurrent() {
    float v_adc_i = readMilliVoltsMedian(PIN_I) / 1000.0f;
    float v_cal_i = v_adc_i - OFF_ADC;
    current = (v_cal_i - VZERO) / SENS;
    
    if (current > -0.15f && current < 0.15f) {
        current = 0.0f;
    }
}

void BMSSensors::readTemperature() {
    float v_temp = readMilliVoltsMedian(PIN_TEMP) / 1000.0f;
    float v_temp_cal = v_temp - TEMP_OFFSET;
    
    if (v_temp_cal < 0) v_temp_cal = 0;
    
    temperature = v_temp_cal / 0.01f;
    
    if (temperature < -20.0f || temperature > 80.0f) {
        temperature = 25.0f;
    }
}

void BMSSensors::readAllSensors() {
    readTaps();
    calculateCellVoltages();
    readCurrent();
    readTemperature();
    lastReadTime = millis();
}

float BMSSensors::getCellVoltage(int cellNum) const {
    if (cellNum >= 1 && cellNum <= 4) {
        return cellVoltages[cellNum - 1];
    }
    return 0.0f;
}

float BMSSensors::getPackVoltage() const {
    return packVoltage;
}

float BMSSensors::getCurrent() const {
    return current;
}

float BMSSensors::getTemperature() const {
    return temperature;
}

unsigned long BMSSensors::getLastReadTime() const {
    return lastReadTime;
}

float BMSSensors::getTap(int tapNum) const {
    switch(tapNum) {
        case 1: return tap1;
        case 2: return tap2;
        case 3: return tap3;
        case 4: return tap4;
        default: return 0.0f;
    }
}

float BMSSensors::getCellImbalance() const {
    float maxV = cellVoltages[0];
    float minV = cellVoltages[0];
    
    for (int i = 1; i < 4; i++) {
        if (cellVoltages[i] > maxV) maxV = cellVoltages[i];
        if (cellVoltages[i] < minV) minV = cellVoltages[i];
    }
    
    return maxV - minV;
}

int BMSSensors::getMaxCellIndex() const {
    float maxV = cellVoltages[0];
    int idx = 0;
    
    for (int i = 1; i < 4; i++) {
        if (cellVoltages[i] > maxV) {
            maxV = cellVoltages[i];
            idx = i;
        }
    }
    
    return idx + 1;
}

float BMSSensors::getMinCellVoltage() const {
    float minV = cellVoltages[0];
    
    for (int i = 1; i < 4; i++) {
        if (cellVoltages[i] < minV) {
            minV = cellVoltages[i];
        }
    }
    
    return minV;
}

float BMSSensors::getMaxCellVoltage() const {
    float maxV = cellVoltages[0];
    
    for (int i = 1; i < 4; i++) {
        if (cellVoltages[i] > maxV) {
            maxV = cellVoltages[i];
        }
    }
    
    return maxV;
}

bool BMSSensors::isCharging() const {
    return current > 0.2f;
}

bool BMSSensors::isDischarging() const {
    return current < -0.2f;
}

bool BMSSensors::isIdle() const {
    return (current >= -0.2f && current <= 0.2f);
}

void BMSSensors::printDebug() {
    Serial.println("\n╔═══ SENSORS DEBUG ═══╗");
    Serial.printf("📦 Pack: %.3fV\n", packVoltage);
    Serial.println("📊 Cells:");
    for (int i = 0; i < 4; i++) {
        Serial.printf("   Cell %d: %.3fV\n", i+1, cellVoltages[i]);
    }
    Serial.printf("⚡ Current: %+.3fA\n", current);
    Serial.printf("🌡  Temp: %.1f°C\n", temperature);
    Serial.printf("⏱  Last read: %lums ago\n", millis() - lastReadTime);
    Serial.println("╚═════════════════════╝\n");
}