#include "soc_estimator.h"

SOCEstimator::SOCEstimator(float capacity_ah) 
    : CAPACITY_AH(capacity_ah),
      CAPACITY_MAH(capacity_ah * 1000.0f),
      soc(50.0f),
      coulombCounter_mAh(capacity_ah * 500.0f),
      lastUpdateTime(0),
      initialized(false),
      idleStartTime(0),
      isIdle(false),
      chargedFullThisCycle(false)
{
}

float SOCEstimator::ocvToSOC(float voltage) {
    if (voltage <= OCV_TABLE[0][1]) return 0.0f;
    if (voltage >= OCV_TABLE[10][1]) return 100.0f;
    
    for (int i = 0; i < 10; i++) {
        float v1 = OCV_TABLE[i][1];
        float v2 = OCV_TABLE[i+1][1];
        
        if (voltage >= v1 && voltage <= v2) {
            float soc1 = OCV_TABLE[i][0];
            float soc2 = OCV_TABLE[i+1][0];
            
            return soc1 + (voltage - v1) * (soc2 - soc1) / (v2 - v1);
        }
    }
    
    return 50.0f;
}

float SOCEstimator::getTempCoeff(float temp) {
    if (temp <= TEMP_COMP_TABLE[0][0]) return TEMP_COMP_TABLE[0][1];
    if (temp >= TEMP_COMP_TABLE[4][0]) return TEMP_COMP_TABLE[4][1];
    
    for (int i = 0; i < 4; i++) {
        float t1 = TEMP_COMP_TABLE[i][0];
        float t2 = TEMP_COMP_TABLE[i+1][0];
        
        if (temp >= t1 && temp <= t2) {
            float a1 = TEMP_COMP_TABLE[i][1];
            float a2 = TEMP_COMP_TABLE[i+1][1];
            
            return a1 + (temp - t1) * (a2 - a1) / (t2 - t1);
        }
    }
    
    return 1.0f;
}

void SOCEstimator::autoRecalibrate(float voltage, float current) {
    if (abs(current) < I_IDLE_THRESHOLD) {
        if (!isIdle) {
            isIdle = true;
            idleStartTime = millis();
        }
    } else {
        isIdle = false;
    }
    
    unsigned long idleDuration = isIdle ? (millis() - idleStartTime) : 0;
    
    if (voltage >= 14.5 && current > 0) {
        chargedFullThisCycle = true;
    }
    if (voltage <= 13.2) {
        chargedFullThisCycle = false;
    }

    if (voltage >= V_RECALIB_FULL &&
        abs(current) < I_IDLE_THRESHOLD &&
        idleDuration >= 1800000 &&
        chargedFullThisCycle) {
        
        if (abs(soc - 100.0f) > 2.0f) {
            Serial.println("Recal: FULL");
        }
        soc = 100.0f;
        coulombCounter_mAh = CAPACITY_MAH;
    }

    if (isIdle && idleDuration > 7200000) {
        float ocvSOC = ocvToSOC(voltage);
        float socError = abs(ocvSOC - soc);
        
        if (socError > 5.0f) {
            float socNew = soc * ALPHA + ocvSOC * (1.0f - ALPHA);
            Serial.printf(
                "OCV Sync (soft): SOC=%.1f%% | OCV=%.1f%% → %.1f%%\n",
                soc, ocvSOC, socNew
            );
            soc = socNew;
            coulombCounter_mAh = (soc / 100.0f) * CAPACITY_MAH;
            idleStartTime = millis();
        }
    }
}

void SOCEstimator::initializeFromVoltage(float packVoltage) {
    if (initialized) return;
    
    soc = ocvToSOC(packVoltage);
    coulombCounter_mAh = (soc / 100.0f) * CAPACITY_MAH;
    lastUpdateTime = millis();
    initialized = true;
    
    Serial.printf("Init: %.3fV → %.1f%% (%.1fAh)\n", 
                  packVoltage, soc, CAPACITY_AH);
}

void SOCEstimator::update(float current_A, float temperature) {
    if (!initialized) {
        Serial.println("SOC not initialized! Call initializeFromVoltage() first");
        return;
    }

    unsigned long now = millis();
    float dt_sec = (now - lastUpdateTime) / 1000.0f;
    lastUpdateTime = now;

    if (dt_sec > 2.0f) dt_sec = 2.0f;

    float charge_mAh = current_A * 1000.0f * (dt_sec / 3600.0f);
    coulombCounter_mAh += charge_mAh;

    float tempCoeff = getTempCoeff(temperature);
    float effectiveCapacity_mAh = CAPACITY_MAH * tempCoeff;

    soc = (coulombCounter_mAh / effectiveCapacity_mAh) * 100.0f;

    if (soc > 100.0f) {
        soc = 100.0f;
    }
    if (soc < 0.0f) {
        soc = 0.0f;
        coulombCounter_mAh = 0.0f;
    }
}

void SOCEstimator::recalibrate(float packVoltage, float current_A) {
    autoRecalibrate(packVoltage, current_A);
}

void SOCEstimator::reset(float newSOC) {
    soc = constrain(newSOC, 0.0f, 100.0f);
    coulombCounter_mAh = (soc / 100.0f) * CAPACITY_MAH;
}

float SOCEstimator::getSOC() const {
    return soc;
}

void SOCEstimator::printDebug(float packVoltage, float current_A, float temperature) {
    float ocvSOC = ocvToSOC(packVoltage);
    float tempCoeff = getTempCoeff(temperature);
    
    Serial.println("\n SOC DEBUG ");
    Serial.printf("SOC: %.1f%% | OCV: %.1f%% (Δ%.1f%%)\n", 
                  soc, ocvSOC, abs(soc - ocvSOC));
    Serial.printf("%.1f/%.0f mAh | 🌡 %.1f°C (α%.2f)\n", 
                  coulombCounter_mAh, CAPACITY_MAH, temperature, tempCoeff);
    Serial.printf(" %s |  %+.2fA\n",
                  isIdle ? "IDLE" : "ACTIVE", current_A);
    
    if (abs(soc - ocvSOC) > 10.0f) {
        Serial.println("Large error - Check calibration");
    }
    
}








