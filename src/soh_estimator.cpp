#include "soh_estimator.h"

SOHEstimator::SOHEstimator(float nominal_capacity_ah) 
    : NOMINAL_CAPACITY_AH(nominal_capacity_ah),
      EOL_CAPACITY_PERCENT(80.0f),
      RATED_CYCLES(2000.0f),
      soh(100.0f),
      totalCycles(0.0f),
      equivalentFullCycles(0.0f),
      currentCapacity_Ah(nominal_capacity_ah),
      lastSOC(50.0f),
      cycleDepthAccum(0.0f),
      chargingCycle(false),
      dischargingCycle(false),
      lastSaveTime(0)
{
}

void SOHEstimator::begin() {
    loadFromFlash();
}

float SOHEstimator::calculateSOHFromCycles(float cycles) {
    float sohCycle = 100.0f - (cycles * CYCLE_AGING_LINEAR);
    
    if (sohCycle < 0.0f) sohCycle = 0.0f;
    if (sohCycle > 100.0f) sohCycle = 100.0f;
    
    return sohCycle;
}

void SOHEstimator::detectCycle(float currentSOC) {
    float deltaSOC = currentSOC - lastSOC;
    
    if (deltaSOC > 0) {
        if (!chargingCycle) {
            chargingCycle = true;
            dischargingCycle = false;
        }
        cycleDepthAccum += deltaSOC;
    }
    else if (deltaSOC < 0) {
        if (!dischargingCycle) {
            dischargingCycle = true;
            chargingCycle = false;
        }
        cycleDepthAccum += abs(deltaSOC);
    }
    
    if (cycleDepthAccum >= 100.0f) {
        float newCycles = cycleDepthAccum / 100.0f;
        equivalentFullCycles += newCycles;
        totalCycles += newCycles;
        cycleDepthAccum = 0.0f;
        
        Serial.printf("🔄 +%.2f cycles | Total: %.1f\n", newCycles, totalCycles);
    }
    
    lastSOC = currentSOC;
}

void SOHEstimator::saveToFlash() {
    prefs.begin(NAMESPACE, false);
    prefs.putFloat("soh", soh);
    prefs.putFloat("cycles", totalCycles);
    prefs.putFloat("eqCycles", equivalentFullCycles);
    prefs.putFloat("capacity", currentCapacity_Ah);
    prefs.end();
}

void SOHEstimator::loadFromFlash() {
    prefs.begin(NAMESPACE, true);
    soh = prefs.getFloat("soh", 100.0f);
    totalCycles = prefs.getFloat("cycles", 0.0f);
    equivalentFullCycles = prefs.getFloat("eqCycles", 0.0f);
    currentCapacity_Ah = prefs.getFloat("capacity", NOMINAL_CAPACITY_AH);
    prefs.end();
    
    Serial.printf("📂 SOH loaded: %.1f%% | %.1f cycles\n", soh, totalCycles);
}

void SOHEstimator::update(float currentSOC, float temperature) {
    unsigned long now = millis();
    
    detectCycle(currentSOC);
    
    soh = calculateSOHFromCycles(totalCycles);
    
    if (soh < 0.0f) soh = 0.0f;
    if (soh > 100.0f) soh = 100.0f;
    
    currentCapacity_Ah = NOMINAL_CAPACITY_AH * (soh / 100.0f);
    
    if (now - lastSaveTime >= SAVE_INTERVAL) {
        saveToFlash();
        lastSaveTime = now;
    }
}

void SOHEstimator::resetCycles() {
    totalCycles = 0.0f;
    equivalentFullCycles = 0.0f;
    cycleDepthAccum = 0.0f;
    saveToFlash();
    Serial.println("🔄 Cycles reset");
}

void SOHEstimator::resetSOH() {
    soh = 100.0f;
    totalCycles = 0.0f;
    equivalentFullCycles = 0.0f;
    currentCapacity_Ah = NOMINAL_CAPACITY_AH;
    saveToFlash();
    Serial.println("🔄 SOH reset to 100%");
}

void SOHEstimator::calibrateFromCapacity(float measured_capacity_Ah) {
    soh = (measured_capacity_Ah / NOMINAL_CAPACITY_AH) * 100.0f;
    currentCapacity_Ah = measured_capacity_Ah;
    
    float estimatedCycles = (100.0f - soh) / CYCLE_AGING_LINEAR;
    totalCycles = estimatedCycles;
    
    saveToFlash();
    Serial.printf("🔧 SOH calibrated: %.1f%% (%.2fAh)\n", soh, currentCapacity_Ah);
}

float SOHEstimator::getSOH() const { 
    return soh; 
}

float SOHEstimator::getTotalCycles() const { 
    return totalCycles; 
}

float SOHEstimator::getEquivalentCycles() const { 
    return equivalentFullCycles; 
}

float SOHEstimator::getCurrentCapacity() const { 
    return currentCapacity_Ah; 
}

float SOHEstimator::getRemainingCycles() const { 
    float cyclesUsed = totalCycles;
    float cyclesRemaining = RATED_CYCLES - cyclesUsed;
    return (cyclesRemaining > 0) ? cyclesRemaining : 0.0f;
}

void SOHEstimator::printDebug() {
    Serial.println("SOH DEBUG (LINEAR MODEL)");
    Serial.printf("SOH: %.1f%%\n", soh);
    Serial.printf("Capacity: %.2f/%.1f Ah\n", currentCapacity_Ah, NOMINAL_CAPACITY_AH);
    Serial.printf("Cycles: %.1f / %.0f (%.1f%% used)\n", 
                  totalCycles, RATED_CYCLES, (totalCycles/RATED_CYCLES)*100.0f);
    Serial.printf("Equiv Cycles: %.2f\n", equivalentFullCycles);
    Serial.printf("Est. Remaining: %.0f cycles\n", getRemainingCycles());
    
    Serial.println("────────────────────────────────");
    Serial.printf("Formula: SOH = 100 - (%.1f × 0.01)\n", totalCycles);
    Serial.printf("           SOH = 100 - %.2f = %.1f%%\n", 
                  totalCycles * 0.01f, 100.0f - totalCycles * 0.01f);
    
    if (soh < 80.0f) {
        Serial.println("Battery approaching EOL!");
    }
    if (totalCycles > RATED_CYCLES * 0.9f) {
        Serial.println(">90% rated cycles used");
    }
    
    Serial.println("╚═════════════════════════════════╝\n");
}

void SOHEstimator::printCompact() {
    Serial.printf("%.1f%% | %.2fAh | %.0f cycles", 
                  soh, currentCapacity_Ah, totalCycles);
}