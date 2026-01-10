#ifndef BMS_SENSORS_H
#define BMS_SENSORS_H

#include <Arduino.h>

class BMSSensors {
private:
    // Cấu hình chân
    const int PIN_T1 = 34;
    const int PIN_T2 = 35;
    const int PIN_T3 = 32;
    const int PIN_T4 = 33;
    const int PIN_I   = 36;
    const int PIN_TEMP = 39;
    const int SAMPLE_COUNT = 21;
    
    // Hiệu chuẩn voltage
    const float OFF1 = 0.027f;
    const float OFF2 = 0.0420f;
    const float OFF3 = 0.0300f;
    const float OFF4 = 0.0280f;
    const float DIV1 = 5.015f;
    const float DIV2 = 5.025f;
    const float DIV3 = 5.015f;
    const float DIV4 = 5.045f;
    
    // Hiệu chuẩn dòng
    const float OFF_ADC = 0.004f;
    const float VZERO   = 2.550f;
    const float SENS    = 0.103f;
    
    // Hiệu chuẩn nhiệt độ
    const float TEMP_OFFSET = 0.024f;
    
    // Dữ liệu đọc được
    float tap1, tap2, tap3, tap4;
    float cellVoltages[4];
    float packVoltage;
    float current;
    float temperature;
    unsigned long lastReadTime;
    
    // Hàm nội bộ
    uint16_t readMilliVoltsMedian(int pin, int n = 21);
    void readTaps();
    void calculateCellVoltages();
    void readCurrent();
    void readTemperature();

public:
    BMSSensors();
    void begin();
    void readAllSensors();
    
    // Getters
    float getCellVoltage(int cellNum) const;
    float getPackVoltage() const;
    float getCurrent() const;
    float getTemperature() const;
    unsigned long getLastReadTime() const;
    float getTap(int tapNum) const;
    float getCellImbalance() const;
    int getMaxCellIndex() const;
    float getMinCellVoltage() const;
    float getMaxCellVoltage() const;
    
    // Status checks
    bool isCharging() const;
    bool isDischarging() const;
    bool isIdle() const;
    
    // Debug
    void printDebug();
};

#endif