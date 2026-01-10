#ifndef SOC_ESTIMATOR_H
#define SOC_ESTIMATOR_H

#include <Arduino.h>

class SOCEstimator {
private:
    // Thông số pin
    const float CAPACITY_AH;
    const float CAPACITY_MAH;
    
    // Ngưỡng hiệu chỉnh
    const float V_FULL = 14.5f;
    const float V_CHARGING = 14.6f;
    const float V_EMPTY = 9.0f;
    const float V_CUTOFF = 10.0f;
    const float V_RECALIB_FULL = 14.6f;
    const float V_RECALIB_EMPTY = 10.0f;
    const float I_IDLE_THRESHOLD = 0.05f;
    const float ALPHA = 0.85f;
    
    // Bảng OCV cho LiFePO4 4S
    const float OCV_TABLE[11][2] = {
        {0,   10.0},
        {10,  12.0},
        {20,  12.5},
        {30,  12.8},
        {40,  12.9},
        {50,  13.0},
        {60,  13.1},
        {70,  13.2},
        {80,  13.3},
        {90,  13.4},
        {100, 13.6}
    };
    
    // Bảng bù nhiệt độ
    const float TEMP_COMP_TABLE[5][2] = {
        {-20, 0.40},
        {-10, 0.60},
        {0,   0.85},
        {25,  1.00},
        {60,  0.98}
    };
    
    // Biến trạng thái
    float soc;
    float coulombCounter_mAh;
    unsigned long lastUpdateTime;
    bool initialized;
    
    unsigned long idleStartTime;
    bool isIdle;
    bool chargedFullThisCycle;
    
    // Hàm nội bộ
    float ocvToSOC(float voltage);
    float getTempCoeff(float temp);
    void autoRecalibrate(float voltage, float current);

public:
    SOCEstimator(float capacity_ah);
    
    void initializeFromVoltage(float packVoltage);
    void update(float current_A, float temperature);
    void recalibrate(float packVoltage, float current_A);
    void reset(float newSOC);
    
    float getSOC() const;
    
    void printDebug(float packVoltage, float current_A, float temperature);
};

#endif