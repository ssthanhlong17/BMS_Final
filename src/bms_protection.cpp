#include "bms_protection.h"

BMSProtection::BMSProtection() {
    chg_ov_fault = false;
    chg_oc_fault = false;
    chg_temp_fault = false;
    chg_ov_recover_timer = 0;
    chg_oc_recover_timer = 0;
    chg_temp_recover_timer = 0;
    
    dsg_uv_fault = false;
    dsg_oc_fault = false;
    dsg_temp_fault = false;
    dsg_uv_recover_timer = 0;
    dsg_oc_recover_timer = 0;
    dsg_temp_recover_timer = 0;
}

void BMSProtection::begin() {
    Serial.println("Initializing BMS Protection...");
    
    pinMode(PIN_CHG, OUTPUT);
    pinMode(PIN_DSG, OUTPUT);
    
    digitalWrite(PIN_CHG, HIGH);
    digitalWrite(PIN_DSG, HIGH);
    
    Serial.println("Protection initialized - MOSFETs enabled");
}

bool BMSProtection::checkChargeOV(float cell1, float cell2, float cell3, float cell4) {
    unsigned long now = millis();
    
    bool ov_trip = (cell1 >= CHG_OV_TRIP) || (cell2 >= CHG_OV_TRIP) ||
                   (cell3 >= CHG_OV_TRIP) || (cell4 >= CHG_OV_TRIP);
    
    bool ov_recover = (cell1 <= CHG_OV_REL) && (cell2 <= CHG_OV_REL) &&
                      (cell3 <= CHG_OV_REL) && (cell4 <= CHG_OV_REL);
    
    if (!chg_ov_fault) {
        if (ov_trip) {
            chg_ov_fault = true;
            Serial.println("CHG OV Protection triggered!");
        }
    } else {
        if (ov_recover) {
            if (chg_ov_recover_timer == 0) {
                chg_ov_recover_timer = now;
            }
            if (now - chg_ov_recover_timer >= CHG_OV_RECOVER_MS) {
                chg_ov_fault = false;
                chg_ov_recover_timer = 0;
                Serial.println("CHG OV Protection recovered");
            }
        } else {
            chg_ov_recover_timer = 0;
        }
    }
    
    return chg_ov_fault;
}

bool BMSProtection::checkChargeOC(float current) {
    unsigned long now = millis();
    
    bool oc_trip = (current >= CHG_OC_TRIP);
    bool oc_recover = (current <= CHG_OC_REL);
    
    if (!chg_oc_fault) {
        if (oc_trip) {
            chg_oc_fault = true;
            Serial.printf("CHG OC Protection: %.2fA\n", current);
        }
    } else {
        if (oc_recover) {
            if (chg_oc_recover_timer == 0) {
                chg_oc_recover_timer = now;
            }
            if (now - chg_oc_recover_timer >= CHG_OC_RECOVER_MS) {
                chg_oc_fault = false;
                chg_oc_recover_timer = 0;
                Serial.println("CHG OC Protection recovered");
            }
        } else {
            chg_oc_recover_timer = 0;
        }
    }
    
    return chg_oc_fault;
}

bool BMSProtection::checkChargeTemp(float temp) {
    unsigned long now = millis();
    
    bool temp_trip = (temp >= CHG_OT_TRIP) || (temp <= CHG_UT_TRIP);
    bool temp_recover = (temp <= CHG_OT_REL) && (temp >= CHG_UT_REL);
    
    if (!chg_temp_fault) {
        if (temp_trip) {
            chg_temp_fault = true;
            Serial.printf("CHG TEMP Protection: %.1f°C\n", temp);
        }
    } else {
        if (temp_recover) {
            if (chg_temp_recover_timer == 0) {
                chg_temp_recover_timer = now;
            }
            if (now - chg_temp_recover_timer >= CHG_TEMP_RECOVER_MS) {
                chg_temp_fault = false;
                chg_temp_recover_timer = 0;
                Serial.println("CHG TEMP Protection recovered");
            }
        } else {
            chg_temp_recover_timer = 0;
        }
    }
    
    return chg_temp_fault;
}

bool BMSProtection::checkDischargeUV(float cell1, float cell2, float cell3, float cell4) {
    unsigned long now = millis();
    
    bool uv_trip = (cell1 <= DSG_UV_TRIP) || (cell2 <= DSG_UV_TRIP) ||
                   (cell3 <= DSG_UV_TRIP) || (cell4 <= DSG_UV_TRIP);
    
    bool uv_recover = (cell1 >= DSG_UV_REL) && (cell2 >= DSG_UV_REL) &&
                      (cell3 >= DSG_UV_REL) && (cell4 >= DSG_UV_REL);
    
    if (!dsg_uv_fault) {
        if (uv_trip) {
            dsg_uv_fault = true;
            Serial.println("DSG UV Protection triggered!");
        }
    } else {
        if (uv_recover) {
            if (dsg_uv_recover_timer == 0) {
                dsg_uv_recover_timer = now;
            }
            if (now - dsg_uv_recover_timer >= DSG_UV_RECOVER_MS) {
                dsg_uv_fault = false;
                dsg_uv_recover_timer = 0;
                Serial.println("DSG UV Protection recovered");
            }
        } else {
            dsg_uv_recover_timer = 0;
        }
    }
    
    return dsg_uv_fault;
}

bool BMSProtection::checkDischargeOC(float current) {
    unsigned long now = millis();
    
    bool oc_trip = (current <= DSG_OC_TRIP);
    bool oc_recover = (current >= DSG_OC_REL);
    
    if (!dsg_oc_fault) {
        if (oc_trip) {
            dsg_oc_fault = true;
            Serial.printf("DSG OC Protection: %.2fA\n", current);
        }
    } else {
        if (oc_recover) {
            if (dsg_oc_recover_timer == 0) {
                dsg_oc_recover_timer = now;
            }
            if (now - dsg_oc_recover_timer >= DSG_OC_RECOVER_MS) {
                dsg_oc_fault = false;
                dsg_oc_recover_timer = 0;
                Serial.println("DSG OC Protection recovered");
            }
        } else {
            dsg_oc_recover_timer = 0;
        }
    }
    
    return dsg_oc_fault;
}

bool BMSProtection::checkDischargeTemp(float temp) {
    unsigned long now = millis();
    
    bool temp_trip = (temp >= DSG_OT_TRIP) || (temp <= DSG_UT_TRIP);
    bool temp_recover = (temp <= DSG_OT_REL) && (temp >= DSG_UT_REL);
    
    if (!dsg_temp_fault) {
        if (temp_trip) {
            dsg_temp_fault = true;
            Serial.printf("DSG TEMP Protection: %.1f°C\n", temp);
        }
    } else {
        if (temp_recover) {
            if (dsg_temp_recover_timer == 0) {
                dsg_temp_recover_timer = now;
            }
            if (now - dsg_temp_recover_timer >= DSG_TEMP_RECOVER_MS) {
                dsg_temp_fault = false;
                dsg_temp_recover_timer = 0;
                Serial.println("DSG TEMP Protection recovered");
            }
        } else {
            dsg_temp_recover_timer = 0;
        }
    }
    
    return dsg_temp_fault;
}

void BMSProtection::update(float cell1, float cell2, float cell3, float cell4, 
                           float current, float temp) {
    bool chg_fault = checkChargeOV(cell1, cell2, cell3, cell4) ||
                     checkChargeOC(current) ||
                     checkChargeTemp(temp);
    
    bool dsg_fault = checkDischargeUV(cell1, cell2, cell3, cell4) ||
                     checkDischargeOC(current) ||
                     checkDischargeTemp(temp);
    
    digitalWrite(PIN_CHG, chg_fault ? LOW : HIGH);
    digitalWrite(PIN_DSG, dsg_fault ? LOW : HIGH);
}

bool BMSProtection::isChargeFault() const {
    return chg_ov_fault || chg_oc_fault || chg_temp_fault;
}

bool BMSProtection::isDischargeFault() const {
    return dsg_uv_fault || dsg_oc_fault || dsg_temp_fault;
}

bool BMSProtection::isAnyFault() const {
    return isChargeFault() || isDischargeFault();
}

bool BMSProtection::getChargeMosfetState() const {
    return digitalRead(PIN_CHG);
}

bool BMSProtection::getDischargeMosfetState() const {
    return digitalRead(PIN_DSG);
}

void BMSProtection::clearProtection() {
    Serial.println("Manually clearing all protections...");
    
    chg_ov_fault = false;
    chg_oc_fault = false;
    chg_temp_fault = false;
    chg_ov_recover_timer = 0;
    chg_oc_recover_timer = 0;
    chg_temp_recover_timer = 0;
    
    dsg_uv_fault = false;
    dsg_oc_fault = false;
    dsg_temp_fault = false;
    dsg_uv_recover_timer = 0;
    dsg_oc_recover_timer = 0;
    dsg_temp_recover_timer = 0;
    
    digitalWrite(PIN_CHG, HIGH);
    digitalWrite(PIN_DSG, HIGH);
    
    Serial.println("Protection cleared");
}

void BMSProtection::printStatus() {
    Serial.println("\n╔═══ PROTECTION STATUS ═══╗");
    Serial.printf("⚡ CHG MOSFET: %s\n", digitalRead(PIN_CHG) ? "ON" : "OFF");
    Serial.printf("⚡ DSG MOSFET: %s\n", digitalRead(PIN_DSG) ? "ON" : "OFF");
    Serial.println("├─────────────────────────┤");
    Serial.println("│ CHARGING PROTECTION:    │");
    Serial.printf("│  OV: %s\n", chg_ov_fault ? "FAULT" : "OK");
    Serial.printf("│  OC: %s\n", chg_oc_fault ? "FAULT" : "OK");
    Serial.printf("│  TEMP: %s\n", chg_temp_fault ? "FAULT" : "OK");
    Serial.println("├─────────────────────────┤");
    Serial.println("│ DISCHARGE PROTECTION:   │");
    Serial.printf("│  UV: %s\n", dsg_uv_fault ? "FAULT" : "OK");
    Serial.printf("│  OC: %s\n", dsg_oc_fault ? "FAULT" : "OK");
    Serial.printf("│  TEMP: %s\n", dsg_temp_fault ? "FAULT" : "OK");
    Serial.println("╚═════════════════════════╝\n");
}