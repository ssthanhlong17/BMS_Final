#ifndef SOH_ESTIMATOR_H
#define SOH_ESTIMATOR_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * ═══════════════════════════════════════════════════════════
 *  SOH ESTIMATOR for LiFePO4 Battery (EVH-32700)
 *  
 *  Methods:
 *  - LINEAR cycle aging model (simple & reliable)
 *  - Cycle counting (charge/discharge cycles)
 *  - Data persistence in NVS flash
 *  
 *  Based on: EVH-32700 datasheet
 *  - Nominal: 6000mAh
 *  - After 2000 cycles: ≥80% capacity (≥4800mAh)
 *  - Linear degradation: 0.01% per cycle
 * ═══════════════════════════════════════════════════════════
 */

class SOHEstimator {
private:
    // ==================== THÔNG SỐ PIN THEO DATASHEET ====================
    const float NOMINAL_CAPACITY_AH;      // 6.0 Ah
    const float EOL_CAPACITY_PERCENT;     // 80% (End of Life)
    const float RATED_CYCLES;             // 2000 cycles @ 80% SOH
    
    // ==================== HỆ SỐ TÍNH TOÁN ====================
    const float CYCLE_AGING_LINEAR = 0.01f;  // Mất 0.01% SOH mỗi cycle
                                              // 2000 cycles × 0.01% = 20% loss
    
    // ==================== BIẾN TRẠNG THÁI ====================
    float soh;                        // SOH hiện tại (%)
    float totalCycles;                // Tổng số chu kỳ
    float equivalentFullCycles;       // Chu kỳ đầy đủ tương đương
    float currentCapacity_Ah;         // Dung lượng hiện tại (Ah)
    
    // Theo dõi chu kỳ
    float lastSOC;                    // SOC trước đó
    float cycleDepthAccum;            // Tích lũy độ sâu chu kỳ
    bool chargingCycle;               // Đang trong chu kỳ sạc
    bool dischargingCycle;            // Đang trong chu kỳ xả
    
    // Persistent storage
    Preferences prefs;
    const char* NAMESPACE = "soh_data";
    unsigned long lastSaveTime;
    const unsigned long SAVE_INTERVAL = 300000;  // 5 phút
    
    // ==================== HÀM NỘI BỘ ====================
    
    /**
     * Tính SOH từ số chu kỳ (mô hình tuyến tính)
     * SOH = 100 - (cycles × 0.01)
     */
    float calculateSOHFromCycles(float cycles) {
        // Mô hình tuyến tính đơn giản
        float sohCycle = 100.0f - (cycles * CYCLE_AGING_LINEAR);
        
        // Giới hạn
        if (sohCycle < 0.0f) sohCycle = 0.0f;
        if (sohCycle > 100.0f) sohCycle = 100.0f;
        
        return sohCycle;
    }
    
    /**
     * Phát hiện chu kỳ sạc/xả và cập nhật (đơn giản hóa)
     */
    void detectCycle(float currentSOC) {
        float deltaSOC = currentSOC - lastSOC;
        
        // ===== PHÁT HIỆN CHU KỲ SẠC =====
        if (deltaSOC > 0) {  // SOC tăng = đang sạc
            if (!chargingCycle) {
                chargingCycle = true;
                dischargingCycle = false;
            }
            cycleDepthAccum += deltaSOC;
        }
        
        // ===== PHÁT HIỆN CHU KỲ XẢ =====
        else if (deltaSOC < 0) {  // SOC giảm = đang xả
            if (!dischargingCycle) {
                dischargingCycle = true;
                chargingCycle = false;
            }
            cycleDepthAccum += abs(deltaSOC);
        }
        
        // ===== TÍNH CHU KỲ TƯƠNG ĐƯƠNG =====
        if (cycleDepthAccum >= 100.0f) {
            float newCycles = cycleDepthAccum / 100.0f;
            equivalentFullCycles += newCycles;
            totalCycles += newCycles;
            cycleDepthAccum = 0.0f;
            
            Serial.printf("🔄 +%.2f cycles | Total: %.1f\n", newCycles, totalCycles);
        }
        
        lastSOC = currentSOC;
    }
    
    /**
     * Lưu dữ liệu vào flash
     */
    void saveToFlash() {
        prefs.begin(NAMESPACE, false);
        prefs.putFloat("soh", soh);
        prefs.putFloat("cycles", totalCycles);
        prefs.putFloat("eqCycles", equivalentFullCycles);
        prefs.putFloat("capacity", currentCapacity_Ah);
        prefs.end();
    }
    
    /**
     * Đọc dữ liệu từ flash
     */
    void loadFromFlash() {
        prefs.begin(NAMESPACE, true);  // Read-only
        soh = prefs.getFloat("soh", 100.0f);
        totalCycles = prefs.getFloat("cycles", 0.0f);
        equivalentFullCycles = prefs.getFloat("eqCycles", 0.0f);
        currentCapacity_Ah = prefs.getFloat("capacity", NOMINAL_CAPACITY_AH);
        prefs.end();
        
        Serial.printf("📂 SOH loaded: %.1f%% | %.1f cycles\n", soh, totalCycles);
    }

public:
    // ==================== CONSTRUCTOR ====================
    SOHEstimator(float nominal_capacity_ah) 
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
    
    // ==================== KHỞI TẠO ====================
    void begin() {
        loadFromFlash();
    }
    
    // ==================== CẬP NHẬT SOH ====================
    void update(float currentSOC, float temperature) {
        unsigned long now = millis();
        
        // ===== 1. PHÁT HIỆN CHU KỲ =====
        detectCycle(currentSOC);
        
        // ===== 2. TÍNH SOH TỪ CHU KỲ (TUYẾN TÍNH) =====
        soh = calculateSOHFromCycles(totalCycles);
        
        // Giới hạn
        if (soh < 0.0f) soh = 0.0f;
        if (soh > 100.0f) soh = 100.0f;
        
        // ===== 3. CẬP NHẬT DUNG LƯỢNG HIỆN TẠI =====
        currentCapacity_Ah = NOMINAL_CAPACITY_AH * (soh / 100.0f);
        
        // ===== 4. LƯU DỮ LIỆU ĐỊNH KỲ =====
        if (now - lastSaveTime >= SAVE_INTERVAL) {
            saveToFlash();
            lastSaveTime = now;
        }
    }
    
    // ==================== HIỆU CHỈNH THỦ CÔNG ====================
    
    /**
     * Đặt lại số chu kỳ
     */
    void resetCycles() {
        totalCycles = 0.0f;
        equivalentFullCycles = 0.0f;
        cycleDepthAccum = 0.0f;
        saveToFlash();
        Serial.println("🔄 Cycles reset");
    }
    
    /**
     * Đặt lại SOH về 100%
     */
    void resetSOH() {
        soh = 100.0f;
        totalCycles = 0.0f;
        equivalentFullCycles = 0.0f;
        currentCapacity_Ah = NOMINAL_CAPACITY_AH;
        saveToFlash();
        Serial.println("🔄 SOH reset to 100%");
    }
    
    /**
     * Hiệu chỉnh SOH thủ công (khi đo dung lượng thực)
     */
    void calibrateFromCapacity(float measured_capacity_Ah) {
        soh = (measured_capacity_Ah / NOMINAL_CAPACITY_AH) * 100.0f;
        currentCapacity_Ah = measured_capacity_Ah;
        
        // Tính ngược số chu kỳ tương đương
        // Từ công thức: SOH = 100 - cycles × 0.01
        // => cycles = (100 - SOH) / 0.01
        float estimatedCycles = (100.0f - soh) / CYCLE_AGING_LINEAR;
        totalCycles = estimatedCycles;
        
        saveToFlash();
        Serial.printf("🔧 SOH calibrated: %.1f%% (%.2fAh)\n", soh, currentCapacity_Ah);
    }
    
    // ==================== GETTERS ====================
    float getSOH() const { return soh; }
    float getTotalCycles() const { return totalCycles; }
    float getEquivalentCycles() const { return equivalentFullCycles; }
    float getCurrentCapacity() const { return currentCapacity_Ah; }
    float getRemainingCycles() const { 
        float cyclesUsed = totalCycles;
        float cyclesRemaining = RATED_CYCLES - cyclesUsed;
        return (cyclesRemaining > 0) ? cyclesRemaining : 0.0f;
    }
    
    // ==================== DEBUG ====================
    void printDebug() {
        Serial.println("\n╔═══ SOH DEBUG (LINEAR MODEL) ═══╗");
        Serial.printf("💚 SOH: %.1f%%\n", soh);
        Serial.printf("🔋 Capacity: %.2f/%.1f Ah\n", currentCapacity_Ah, NOMINAL_CAPACITY_AH);
        Serial.printf("🔄 Cycles: %.1f / %.0f (%.1f%% used)\n", 
                      totalCycles, RATED_CYCLES, (totalCycles/RATED_CYCLES)*100.0f);
        Serial.printf("⚡ Equiv Cycles: %.2f\n", equivalentFullCycles);
        Serial.printf("📅 Est. Remaining: %.0f cycles\n", getRemainingCycles());
        
        // Công thức
        Serial.println("────────────────────────────────");
        Serial.printf("📐 Formula: SOH = 100 - (%.1f × 0.01)\n", totalCycles);
        Serial.printf("           SOH = 100 - %.2f = %.1f%%\n", 
                      totalCycles * 0.01f, 100.0f - totalCycles * 0.01f);
        
        // Cảnh báo
        if (soh < 80.0f) {
            Serial.println("⚠️  Battery approaching EOL!");
        }
        if (totalCycles > RATED_CYCLES * 0.9f) {
            Serial.println("⚠️  >90% rated cycles used");
        }
        
        Serial.println("╚═════════════════════════════════╝\n");
    }
    
    /**
     * Thông tin ngắn gọn
     */
    void printCompact() {
        Serial.printf("💚 %.1f%% | 🔋 %.2fAh | 🔄 %.0f cycles", 
                      soh, currentCapacity_Ah, totalCycles);
    }
};

#endif // SOH_ESTIMATOR_H