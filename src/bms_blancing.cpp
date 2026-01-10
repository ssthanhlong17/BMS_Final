#include "bms_balancing.h"

// ========================= CONSTRUCTOR =========================
BMSBalancing::BMSBalancing() {
    bal_active = false;
    bal_timer = 0;
    bal_on_phase = false;
    bal_cell = 0;
}

// ========================= KHỞI TẠO =========================
void BMSBalancing::begin() {
    
    pinMode(PIN_BAL1, OUTPUT);
    pinMode(PIN_BAL2, OUTPUT);
    pinMode(PIN_BAL3, OUTPUT);
    pinMode(PIN_BAL4, OUTPUT);
    
    balanceAllOff();
}

// ========================= HÀM NỘI BỘ =========================
void BMSBalancing::balanceAllOff() {
    digitalWrite(PIN_BAL1, LOW);
    digitalWrite(PIN_BAL2, LOW);
    digitalWrite(PIN_BAL3, LOW);
    digitalWrite(PIN_BAL4, LOW);
}

void BMSBalancing::balanceEnable(uint8_t cell) {
    balanceAllOff();
    
    switch (cell) {
        case 1: digitalWrite(PIN_BAL1, HIGH); break;
        case 2: digitalWrite(PIN_BAL2, HIGH); break;
        case 3: digitalWrite(PIN_BAL3, HIGH); break;
        case 4: digitalWrite(PIN_BAL4, HIGH); break;
    }
}

uint8_t BMSBalancing::getMaxCellIndex(float cell1, float cell2, float cell3, float cell4) {
    float vmax = cell1;
    uint8_t idx = 1;
    
    if (cell2 > vmax) { vmax = cell2; idx = 2; }
    if (cell3 > vmax) { vmax = cell3; idx = 3; }
    if (cell4 > vmax) { vmax = cell4; idx = 4; }
    
    return idx;
}

float BMSBalancing::getMinCellVoltage(float cell1, float cell2, float cell3, float cell4) {
    float vmin = cell1;
    
    if (cell2 < vmin) vmin = cell2;
    if (cell3 < vmin) vmin = cell3;
    if (cell4 < vmin) vmin = cell4;
    
    return vmin;
}

// ========================= CẬP NHẬT CÂN BẰNG =========================
void BMSBalancing::update(float cell1, float cell2, float cell3, float cell4, float current) {
    unsigned long now = millis();
    
    bool idle = (fabs(current) < BAL_IDLE_CURRENT);
    float vmin = getMinCellVoltage(cell1, cell2, cell3, cell4);
    uint8_t vmax_idx = getMaxCellIndex(cell1, cell2, cell3, cell4);
    
    float vmax = 0;
    switch (vmax_idx) {
        case 1: vmax = cell1; break;
        case 2: vmax = cell2; break;
        case 3: vmax = cell3; break;
        case 4: vmax = cell4; break;
    }
    
    float delta = vmax - vmin;
    
    bool start_cond = idle && (vmax >= BAL_MIN_CELL_V) && (delta >= BAL_DELTA_START);
    bool stop_cond = (!idle) || (delta <= BAL_DELTA_STOP);
    
    if (!bal_active) {
        if (start_cond) {
            bal_active = true;
            bal_on_phase = true;
            bal_cell = vmax_idx;
            bal_timer = now;
            balanceEnable(bal_cell);
            
            Serial.printf("Balancing started: Cell %d (%.3fV vs %.3fV = %.3fV)\n", 
                         bal_cell, vmax, vmin, delta);
        }
    } 
    else {
        if (stop_cond) {
            bal_active = false;
            bal_on_phase = false;
            bal_timer = 0;
            balanceAllOff();
            
            Serial.println("Balancing stopped");
        } 
        else {
            if (bal_on_phase) {
                if (now - bal_timer >= BAL_ON_TIME) {
                    bal_on_phase = false;
                    bal_timer = now;
                    balanceAllOff();
                }
            } 
            else {
                if (now - bal_timer >= BAL_OFF_TIME) {
                    bal_on_phase = true;
                    bal_timer = now;
                    bal_cell = getMaxCellIndex(cell1, cell2, cell3, cell4);
                    balanceEnable(bal_cell);
                }
            }
        }
    }
}

// ========================= GETTERS =========================
bool BMSBalancing::isActive() const {
    return bal_active;
}

bool BMSBalancing::isBalancing(uint8_t cell) const {
    if (!bal_active || !bal_on_phase) return false;
    return (bal_cell == cell);
}

uint8_t BMSBalancing::getBalancingCell() const {
    return bal_active ? bal_cell : 0;
}

// ========================= STOP THỦ CÔNG =========================
void BMSBalancing::stop() {
    bal_active = false;
    bal_on_phase = false;
    bal_timer = 0;
    balanceAllOff();
    Serial.println("Balancing manually stopped");
}

// ========================= DEBUG =========================
void BMSBalancing::printStatus() {
    Serial.println("\n╔═══ BALANCING STATUS ═══╗");
    Serial.printf("Active: %s\n", bal_active ? "YES" : "NO");
    
    if (bal_active) {
        Serial.printf("Cell: %d\n", bal_cell);
        Serial.printf("Phase: %s\n", bal_on_phase ? "ON" : "OFF");
        
        unsigned long elapsed = millis() - bal_timer;
        unsigned long remaining = bal_on_phase ? 
                                 (BAL_ON_TIME - elapsed) : 
                                 (BAL_OFF_TIME - elapsed);
        Serial.printf("⏱  Next switch: %.1fs\n", remaining / 1000.0f);
    }
    
    Serial.println("├─────────────────────────┤");
    Serial.printf("│ BAL1: %s\n", digitalRead(PIN_BAL1) ? "ON" : "OFF");
    Serial.printf("│ BAL2: %s\n", digitalRead(PIN_BAL2) ? "ON" : "OFF");
    Serial.printf("│ BAL3: %s\n", digitalRead(PIN_BAL3) ? "ON" : "OFF");
    Serial.printf("│ BAL4: %s\n", digitalRead(PIN_BAL4) ? "ON" : "OFF");
    Serial.println("╚═════════════════════════╝\n");
}

void BMSBalancing::getBalancingStatus(bool cells[4]) {
    for (int i = 0; i < 4; i++) {
        cells[i] = isBalancing(i + 1);
    }
}