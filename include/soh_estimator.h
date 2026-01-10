#ifndef SOH_ESTIMATOR_H
#define SOH_ESTIMATOR_H

#include <Arduino.h>
#include <Preferences.h>

class SOHEstimator {
private:
    // Thông số pin theo datasheet
    const float NOMINAL_CAPACITY_AH;
    const float EOL_CAPACITY_PERCENT;
    const float RATED_CYCLES;
    
    // Hệ số tính toán
    const float CYCLE_AGING_LINEAR = 0.01f;
    
    // Biến trạng thái
    float soh;
    float totalCycles;
    float equivalentFullCycles;
    float currentCapacity_Ah;
    
    // Theo dõi chu kỳ
    float lastSOC;
    float cycleDepthAccum;
    bool chargingCycle;
    bool dischargingCycle;
    
    // Persistent storage
    Preferences prefs;
    const char* NAMESPACE = "soh_data";
    unsigned long lastSaveTime;
    const unsigned long SAVE_INTERVAL = 300000;
    
    // Hàm nội bộ
    float calculateSOHFromCycles(float cycles);
    void detectCycle(float currentSOC);
    void saveToFlash();
    void loadFromFlash();

public:
    SOHEstimator(float nominal_capacity_ah);
    
    void begin();
    void update(float currentSOC, float temperature);
    
    // Hiệu chỉnh thủ công
    void resetCycles();
    void resetSOH();
    void calibrateFromCapacity(float measured_capacity_Ah);
    
    // Getters
    float getSOH() const;
    float getTotalCycles() const;
    float getEquivalentCycles() const;
    float getCurrentCapacity() const;
    float getRemainingCycles() const;
    
    // Debug
    void printDebug();
    void printCompact();
};

#endif