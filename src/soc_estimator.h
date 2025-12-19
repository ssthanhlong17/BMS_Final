#ifndef SOC_ESTIMATOR_H
#define SOC_ESTIMATOR_H

#include <Arduino.h>

/**
 * ═══════════════════════════════════════════════════════════
 *  SOC ESTIMATOR for LiFePO4 4S Battery Pack
 *  
 *  Method: Hybrid approach
 *  - OCV lookup for initial SOC
 *  - Coulomb counting for realtime tracking
 *  - Temperature compensation
 *  - Automatic recalibration at full/empty states
 * ═══════════════════════════════════════════════════════════
 */

class SOCEstimator {
private:
    // ==================== THÔNG SỐ PIN ====================
    const float CAPACITY_AH;              // Dung lượng pin (Ah)
    const float CAPACITY_MAH;             // Dung lượng pin (mAh)
    
    // ==================== NGƯỠNG HIỆU CHỈNH ====================
    const float V_FULL = 14.5f;           // Pack đầy: 3.625V/cell × 4
    const float V_CHARGING = 14.6f;       // Pack đang sạc max: 3.65V/cell × 4
    const float V_EMPTY = 9.0f;           // Pack cạn: 2.25V/cell × 4
    const float V_CUTOFF = 10.0f;         // Cut-off bảo vệ: 2.5V/cell × 4
    const float V_RECALIB_FULL = 14.6f;   // Ngưỡng bắt đầu kiểm tra đầy
    const float V_RECALIB_EMPTY = 10.0f;  // Ngưỡng bắt đầu kiểm tra cạn
    const float I_IDLE_THRESHOLD = 0.05f;  // Dòng idle (A)
    const float ALPHA = 0.85f;   // Hệ số tin Coulomb (có thể tinh chỉnh)
    
    // ==================== BẢNG OCV CHO LiFePO4 4S ====================
    // [SOC%, Voltage_Pack] - Lấy giá trị giữa của mỗi khoảng × 4 cells
    const float OCV_TABLE[11][2] = {
        {0,   10.0},   // 0%  - pin cạn sâu
        {10,  12.0},   // 10%
        {20,  12.5},   // 20%
        {30,  12.8},   // 30%
        {40,  12.9},   // 40%
        {50,  13.0},   // 50%
        {60,  13.1},   // 60%
        {70,  13.2},   // 70%
        {80,  13.3},   // 80%
        {90,  13.4},   // 90%
        {100, 13.6}    // 100% Rest
    };
    
    // ==================== BẢNG BÙ NHIỆT ĐỘ ====================
    // [Temperature°C, Capacity_Factor]
    // Theo datasheet LiFePO4: dung lượng giảm ở nhiệt độ thấp
    const float TEMP_COMP_TABLE[5][2] = {
        {-20, 0.40},    // -20°C: 40% dung lượng
        {-10, 0.60},    // -10°C: 60% dung lượng
        {0,   0.85},    // 0°C:   85% dung lượng
        {25,  1.00},    // 25°C:  100% dung lượng (chuẩn)
        {60,  0.98}     // 60°C:  98% dung lượng
    };
    
    // ==================== BIẾN TRẠNG THÁI ====================
    float soc;                      // SOC hiện tại (%)
    float coulombCounter_mAh;       // Tích lũy Coulomb (mAh)
    unsigned long lastUpdateTime;   // Thời điểm cập nhật cuối (ms)
    bool initialized;               // Đã khởi tạo SOC chưa
    
    // Biến phụ cho recalibration
    unsigned long idleStartTime;    // Thời điểm bắt đầu idle
    bool isIdle;                    // Đang ở trạng thái idle
    bool chargedFullThisCycle;      // = true khi đã kết thúc sạc đầy trong chu kỳ gần nhất
    
    // ==================== HÀM NỘI BỘ ====================
    
    /**
     * Tính SOC từ OCV bằng nội suy tuyến tính
     */
    float ocvToSOC(float voltage) {
        // Giới hạn điện áp
        if (voltage <= OCV_TABLE[0][1]) return 0.0f;
        if (voltage >= OCV_TABLE[10][1]) return 100.0f;
        
        // Tìm khoảng phù hợp và nội suy
        for (int i = 0; i < 10; i++) {
            float v1 = OCV_TABLE[i][1];
            float v2 = OCV_TABLE[i+1][1];
            
            if (voltage >= v1 && voltage <= v2) {
                float soc1 = OCV_TABLE[i][0];
                float soc2 = OCV_TABLE[i+1][0];
                
                // Nội suy tuyến tính
                return soc1 + (voltage - v1) * (soc2 - soc1) / (v2 - v1);
            }
        }
        
        return 50.0f;  // Fallback
    }
    
    /**
     * Lấy hệ số bù nhiệt độ
     */
    float getTempCoeff(float temp) {
        // Giới hạn nhiệt độ
        if (temp <= TEMP_COMP_TABLE[0][0]) return TEMP_COMP_TABLE[0][1];  // < -20°C
        if (temp >= TEMP_COMP_TABLE[4][0]) return TEMP_COMP_TABLE[4][1];  // > 60°C
        
        // Nội suy tuyến tính giữa các điểm
        for (int i = 0; i < 4; i++) {
            float t1 = TEMP_COMP_TABLE[i][0];
            float t2 = TEMP_COMP_TABLE[i+1][0];
            
            if (temp >= t1 && temp <= t2) {
                float a1 = TEMP_COMP_TABLE[i][1];
                float a2 = TEMP_COMP_TABLE[i+1][1];
                
                // Nội suy: α = a1 + (temp - t1) × (a2 - a1) / (t2 - t1)
                return a1 + (temp - t1) * (a2 - a1) / (t2 - t1);
            }
        }
        
        return 1.0f;  // Fallback
    }
    
    /**
     * Kiểm tra và thực hiện hiệu chỉnh tự động
     */
    void autoRecalibrate(float voltage, float current) {
        // ===== Phát hiện trạng thái IDLE =====
        if (abs(current) < I_IDLE_THRESHOLD) {
            if (!isIdle) {
                isIdle = true;
                idleStartTime = millis();
            }
        } else {
            isIdle = false;
        }
        
        unsigned long idleDuration = isIdle ? (millis() - idleStartTime) : 0;
        
        // ===== Đánh dấu pin đã từng sạc đầy trong chu kỳ hiện tại =====
        if (voltage >= 14.5 && current > 0) {
            chargedFullThisCycle = true;
        }
        if (voltage <= 13.2) {   // ~3.3V/cell - reset cờ khi xả sâu
            chargedFullThisCycle = false;
        }

        // =========================
        //  HIỆU CHỈNH KHI PIN ĐẦY
        // =========================
        // Điều kiện: V ≥ 14.6V, idle ≥ 30 phút, đã từng sạc đầy
        if (voltage >= V_RECALIB_FULL &&
            abs(current) < I_IDLE_THRESHOLD &&
            idleDuration >= 1800000 &&  // 30 phút
            chargedFullThisCycle) {
            
            if (abs(soc - 100.0f) > 2.0f) {
                Serial.println("🔄 Recal: FULL");
            }
            soc = 100.0f;
            coulombCounter_mAh = CAPACITY_MAH;
        }
  
        // ===== HIỆU CHỈNH ĐỊNH KỲ TỪ OCV =====
        // Khi idle > 2 giờ, đồng bộ lại với OCV
        if (isIdle && idleDuration > 7200000) {  // 2 giờ
            float ocvSOC = ocvToSOC(voltage);
            float socError = abs(ocvSOC - soc);
            
            if (socError > 5.0f) {
                float socNew = soc * ALPHA + ocvSOC * (1.0f - ALPHA);
                Serial.printf(
                    "🔄 OCV Sync (soft): SOC=%.1f%% | OCV=%.1f%% → %.1f%%\n",
                    soc, ocvSOC, socNew
                );
                soc = socNew;
                coulombCounter_mAh = (soc / 100.0f) * CAPACITY_MAH;
                idleStartTime = millis();  // Reset để tránh sync lại ngay
            }
        }
    }

public:
    // ==================== CONSTRUCTOR ====================
    SOCEstimator(float capacity_ah) 
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
    
    // ==================== KHỞI TẠO SOC TỪ ĐIỆN ÁP ====================
    void initializeFromVoltage(float packVoltage) {
        if (initialized) return;
        
        soc = ocvToSOC(packVoltage);
        coulombCounter_mAh = (soc / 100.0f) * CAPACITY_MAH;
        lastUpdateTime = millis();
        initialized = true;
        
        Serial.printf("🔋 Init: %.3fV → %.1f%% (%.1fAh)\n", 
                      packVoltage, soc, CAPACITY_AH);
    }
    
    // ==================== CẬP NHẬT SOC (COULOMB COUNTING) ====================
    void update(float current_A, float temperature) {
        if (!initialized) {
            Serial.println("⚠️ SOC not initialized! Call initializeFromVoltage() first");
            return;
        }

        unsigned long now = millis();
        float dt_sec = (now - lastUpdateTime) / 1000.0f;
        lastUpdateTime = now;

        // Giới hạn dt để tránh mất Coulomb khi reset hoặc treo
        if (dt_sec > 2.0f) dt_sec = 2.0f;

        // ===== TÍCH LŨY COULOMB =====
        float charge_mAh = current_A * 1000.0f * (dt_sec / 3600.0f);
        coulombCounter_mAh += charge_mAh;

        // ===== BÙ NHIỆT ĐỘ VÀO DUNG LƯỢNG =====
        float tempCoeff = getTempCoeff(temperature);
        float effectiveCapacity_mAh = CAPACITY_MAH * tempCoeff;

        // ===== TÍNH SOC =====
        soc = (coulombCounter_mAh / effectiveCapacity_mAh) * 100.0f;

        // ===== GIỚI HẠN 0-100% =====
        if (soc > 100.0f) {
            soc = 100.0f;
            // Giữ coulombCounter không đổi để tránh nhảy giá trị
        }
        if (soc < 0.0f) {
            soc = 0.0f;
            coulombCounter_mAh = 0.0f;  // Chỉ reset khi thực sự cạn
        }
    }
    
    // ==================== AUTO RECALIBRATION ====================
    void recalibrate(float packVoltage, float current_A) {
        autoRecalibrate(packVoltage, current_A);
    }
    
    // ==================== HIỆU CHỈNH THỦ CÔNG ====================
    void reset(float newSOC) {
        soc = constrain(newSOC, 0.0f, 100.0f);
        coulombCounter_mAh = (soc / 100.0f) * CAPACITY_MAH;
    }
    
    // ==================== LẤY SOC HIỆN TẠI ====================
    float getSOC() const {
        return soc;
    }
    
    // ==================== DEBUG THÔNG TIN (TÙY CHỌN) ====================
    void printDebug(float packVoltage, float current_A, float temperature) {
        float ocvSOC = ocvToSOC(packVoltage);
        float tempCoeff = getTempCoeff(temperature);
        
        Serial.println("\n╔═══ SOC DEBUG ═══╗");
        Serial.printf("🔋 SOC: %.1f%% | OCV: %.1f%% (Δ%.1f%%)\n", 
                      soc, ocvSOC, abs(soc - ocvSOC));
        Serial.printf("⚡ %.1f/%.0f mAh | 🌡 %.1f°C (α%.2f)\n", 
                      coulombCounter_mAh, CAPACITY_MAH, temperature, tempCoeff);
        Serial.printf("📡 %s | ⚡ %+.2fA\n",
                      isIdle ? "IDLE" : "ACTIVE", current_A);
        
        if (abs(soc - ocvSOC) > 10.0f) {
            Serial.println("⚠️  Large error - Check calibration");
        }
        Serial.println("╚═════════════════╝\n");
    }
};

#endif // SOC_ESTIMATOR_H