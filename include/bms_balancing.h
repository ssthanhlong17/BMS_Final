#ifndef BMS_BALANCING_H
#define BMS_BALANCING_H

#include <Arduino.h>

/**
 * ═══════════════════════════════════════════════════════════
 *  BMS BALANCING MODULE
 *  Cân bằng cell với chu kỳ ON/OFF để tránh quá nhiệt
 * ═══════════════════════════════════════════════════════════
 */

class BMSBalancing {
private:
    // ========================= CẤU HÌNH CHÂN =========================
    const int PIN_BAL1 = 25;
    const int PIN_BAL2 = 26;
    const int PIN_BAL3 = 27;
    const int PIN_BAL4 = 14;
    
    // ========================= NGƯỠNG CÂN BẰNG =========================
    const float BAL_IDLE_CURRENT = 0.1f;
    const float BAL_DELTA_START = 0.1f;
    const float BAL_DELTA_STOP  = 0.03f;
    const float BAL_MIN_CELL_V  = 3.50f;
    
    const unsigned long BAL_ON_TIME  = 5000;
    const unsigned long BAL_OFF_TIME = 5000;
    
    // ========================= TRẠNG THÁI CÂN BẰNG =========================
    bool bal_active;
    unsigned long bal_timer;
    bool bal_on_phase;
    uint8_t bal_cell;
    
    // ========================= HÀM NỘI BỘ =========================
    void balanceAllOff();
    void balanceEnable(uint8_t cell);
    uint8_t getMaxCellIndex(float cell1, float cell2, float cell3, float cell4);
    float getMinCellVoltage(float cell1, float cell2, float cell3, float cell4);

public:
    // ========================= CONSTRUCTOR =========================
    BMSBalancing();
    
    // ========================= KHỞI TẠO =========================
    void begin();
    
    // ========================= CẬP NHẬT CÂN BẰNG =========================
    void update(float cell1, float cell2, float cell3, float cell4, float current);
    
    // ========================= GETTERS =========================
    bool isActive() const;
    bool isBalancing(uint8_t cell) const;
    uint8_t getBalancingCell() const;
    
    // ========================= STOP THỦ CÔNG =========================
    void stop();
    
    // ========================= DEBUG =========================
    void printStatus();
    void getBalancingStatus(bool cells[4]);
};

#endif // BMS_BALANCING_H