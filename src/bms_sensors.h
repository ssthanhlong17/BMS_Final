#ifndef BMS_SENSORS_H
#define BMS_SENSORS_H

#include <Arduino.h>

/**
 * ═══════════════════════════════════════════════════════════
 *  BMS SENSORS MODULE
 *  Xử lý đọc tất cả cảm biến: Voltage, Current, Temperature
 * ═══════════════════════════════════════════════════════════
 */

class BMSSensors {
private:
    // ========================= CẤU HÌNH CHÂN =========================
    const int PIN_T1 = 34;
    const int PIN_T2 = 35;
    const int PIN_T3 = 32;
    const int PIN_T4 = 33;
    const int PIN_I   = 36;
    const int PIN_TEMP = 39;
    
    const int SAMPLE_COUNT = 21;
    
    // ========================= HIỆU CHUẨN VOLTAGE =========================
    const float OFF1 = 0.027f;
    const float OFF2 = 0.0420f;
    const float OFF3 = 0.0300f;
    const float OFF4 = 0.0280f;
    
    const float DIV1 = 5.015f;
    const float DIV2 = 5.025f;
    const float DIV3 = 5.015f;
    const float DIV4 = 5.045f;
    
    // ========================= HIỆU CHUẨN DÒNG (ACS712-20A) =========================
    const float OFF_ADC = 0.024f;
    const float VZERO   = 2.550f;
    const float SENS    = 0.103f;
    
    // ========================= HIỆU CHUẨN NHIỆT ĐỘ (LM35) =========================
    const float TEMP_OFFSET = 0.024f;
    
    // ========================= DỮ LIỆU ĐỌC ĐƯỢC =========================
    float cellVoltages[4];
    float packVoltage;
    float current;
    float temperature;
    
    unsigned long lastReadTime;
    
    // ========================= HÀM LỌC MEDIAN =========================
    uint16_t readMilliVoltsMedian(int pin, int n = 21) {
        static uint16_t buf[64];
        if (n > 64) n = 64;
        
        for (int i = 0; i < n; i++) {
            buf[i] = analogReadMilliVolts(pin);
            delayMicroseconds(200);
        }
        
        // Insertion sort
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
    
    // ========================= ĐỌC ĐIỆN ÁP CELLS =========================
    void readVoltages() {
        // Đọc ADC
        float adc1 = readMilliVoltsMedian(PIN_T1) / 1000.0f;
        float adc2 = readMilliVoltsMedian(PIN_T2) / 1000.0f;
        float adc3 = readMilliVoltsMedian(PIN_T3) / 1000.0f;
        float adc4 = readMilliVoltsMedian(PIN_T4) / 1000.0f;
        
        // Hiệu chỉnh offset
        float vcal1 = adc1 - OFF1;
        float vcal2 = adc2 - OFF2;
        float vcal3 = adc3 - OFF3;
        float vcal4 = adc4 - OFF4;
        
        // Nhân với hệ số chia áp
        float tap1 = vcal1 * DIV1;
        float tap2 = vcal2 * DIV2;
        float tap3 = vcal3 * DIV3;
        float tap4 = vcal4 * DIV4;
        
        // Tính điện áp từng cell
        cellVoltages[0] = tap1 - tap2;  // Cell 1
        cellVoltages[1] = tap2 - tap3;  // Cell 2
        cellVoltages[2] = tap3 - tap4;  // Cell 3
        cellVoltages[3] = tap4;         // Cell 4
        
        // Tổng điện áp pack
        packVoltage = cellVoltages[0] + cellVoltages[1] + 
                      cellVoltages[2] + cellVoltages[3];
    }
    
    // ========================= ĐỌC DÒNG ĐIỆN =========================
    void readCurrent() {
        float v_adc_i = readMilliVoltsMedian(PIN_I) / 1000.0f;
        float v_cal_i = v_adc_i - OFF_ADC;
        current = (v_cal_i - VZERO) / SENS;
    }
    
    // ========================= ĐỌC NHIỆT ĐỘ =========================
    void readTemperature() {
        float v_temp = readMilliVoltsMedian(PIN_TEMP) / 1000.0f;
        float v_temp_cal = v_temp - TEMP_OFFSET;
        
        if (v_temp_cal < 0) v_temp_cal = 0;
        
        temperature = v_temp_cal / 0.01f;  // LM35: 10mV/°C
        
        // Giới hạn hợp lý
        if (temperature < 0 || temperature > 100) {
            temperature = 25.0;  // Default
        }
    }

public:
    // ========================= CONSTRUCTOR =========================
    BMSSensors() {
        for (int i = 0; i < 4; i++) {
            cellVoltages[i] = 0.0f;
        }
        packVoltage = 0.0f;
        current = 0.0f;
        temperature = 25.0f;
        lastReadTime = 0;
    }
    
    // ========================= KHỞI TẠO =========================
    void begin() {
        Serial.println("🔌 Initializing BMS Sensors...");
        
        // Cấu hình ADC
        analogReadResolution(12);
        analogSetPinAttenuation(PIN_T1, ADC_11db);
        analogSetPinAttenuation(PIN_T2, ADC_11db);
        analogSetPinAttenuation(PIN_T3, ADC_11db);
        analogSetPinAttenuation(PIN_T4, ADC_11db);
        analogSetPinAttenuation(PIN_I, ADC_11db);
        analogSetPinAttenuation(PIN_TEMP, ADC_11db);
        
        delay(100);
        
        // Đọc lần đầu
        readAllSensors();
        
        Serial.println("✅ BMS Sensors initialized");
        Serial.printf("   Initial Pack Voltage: %.2fV\n", packVoltage);
        Serial.printf("   Initial Temperature: %.1f°C\n", temperature);
    }
    
    // ========================= ĐỌC TẤT CẢ CẢM BIẾN =========================
    void readAllSensors() {
        readVoltages();
        readCurrent();
        readTemperature();
        lastReadTime = millis();
    }
    
    // ========================= GETTERS =========================
    float getCellVoltage(int cellNum) const {
        if (cellNum >= 1 && cellNum <= 4) {
            return cellVoltages[cellNum - 1];
        }
        return 0.0f;
    }
    
    float getPackVoltage() const {
        return packVoltage;
    }
    
    float getCurrent() const {
        return current;
    }
    
    float getTemperature() const {
        return temperature;
    }
    
    unsigned long getLastReadTime() const {
        return lastReadTime;
    }
    
    // ========================= DEBUG =========================
    void printDebug() {
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
    
    // ========================= KIỂM TRA KHOẢNG CÁCH CÂN BẰNG =========================
    float getCellImbalance() const {
        float maxV = cellVoltages[0];
        float minV = cellVoltages[0];
        
        for (int i = 1; i < 4; i++) {
            if (cellVoltages[i] > maxV) maxV = cellVoltages[i];
            if (cellVoltages[i] < minV) minV = cellVoltages[i];
        }
        
        return maxV - minV;
    }
    
    // ========================= KIỂM TRA TRẠNG THÁI PIN =========================
    bool isCharging() const {
        return current > 0.2f;
    }
    
    bool isDischarging() const {
        return current < -0.2f;
    }
    
    bool isIdle() const {
        return (current >= -0.2f && current <= 0.2f);
    }
};

#endif // BMS_SENSORS_H


