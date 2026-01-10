#include "bms_dwin.h"

BMSDwin::BMSDwin() {
}

void BMSDwin::begin() {
    Serial.println("Initializing DWIN Display...");
    
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    
    delay(100);
    
    writeWord(VP_WARN_CHG_OV, WARN_ID_NORMAL);
    writeWord(VP_WARN_CHG_OC, WARN_ID_NORMAL);
    writeWord(VP_WARN_CHG_TEMP, WARN_ID_NORMAL);
    writeWord(VP_WARN_DSG_UV, WARN_ID_NORMAL);
    writeWord(VP_WARN_DSG_OC, WARN_ID_NORMAL);
    writeWord(VP_WARN_DSG_TEMP, WARN_ID_NORMAL);
    
    Serial.println("DWIN Display initialized");
}

void BMSDwin::writeWord(uint16_t vp, int16_t value) {
    uint8_t frame[8];
    frame[0] = 0x5A;
    frame[1] = 0xA5;
    frame[2] = 0x05;
    frame[3] = 0x82;
    frame[4] = vp >> 8;
    frame[5] = vp & 0xFF;
    frame[6] = value >> 8;
    frame[7] = value & 0xFF;
    
    Serial2.write(frame, 8);
}

void BMSDwin::writeFloat(uint16_t vp, float value, uint8_t decimal_places) {
    uint16_t scale = 1;
    for (uint8_t i = 0; i < decimal_places; i++) {
        scale *= 10;
    }
    
    int16_t scaled_value = (int16_t)(value * scale);
    writeWord(vp, scaled_value);
}

void BMSDwin::sendVoltages(float cell1, float cell2, float cell3, float cell4, float pack) {
    writeFloat(VP_CELL1, cell1, 3);
    writeFloat(VP_CELL2, cell2, 3);
    writeFloat(VP_CELL3, cell3, 3);
    writeFloat(VP_CELL4, cell4, 3);
    writeFloat(VP_PACK, pack, 3);
}

void BMSDwin::sendCurrent(float current) {
    float valueToSend;
    
    if (current > -0.15f && current < 0.15f) {
        valueToSend = 0.0f;
    } else {
        valueToSend = current;
    }
    
    writeFloat(VP_CURRENT, valueToSend, 3);
}

void BMSDwin::sendCurrentIcon(float current) {
    if (current > -0.15f && current < 0.15f) {
        writeWord(VP_ICON_CURRENT, ICON_IDLE);
    } 
    else if (current >= 0.3f) {
        writeWord(VP_ICON_CURRENT, ICON_CHARGING);
    } 
    else if (current <= -0.3f) {
        writeWord(VP_ICON_CURRENT, ICON_DISCHARGING);
    }
}

void BMSDwin::sendTemperature(float temp) {
    writeFloat(VP_TEMP, temp, 2);
}

void BMSDwin::updateBasicData(float cell1, float cell2, float cell3, float cell4,
                    float pack, float current, float temp) {
    sendVoltages(cell1, cell2, cell3, cell4, pack);
    sendCurrent(current);
    sendCurrentIcon(current);
    sendTemperature(temp);
}

void BMSDwin::updateWarningChgOV(bool warning, bool alarm) {
    if (alarm) {
        writeWord(VP_WARN_CHG_OV, WARN_ID_CHG_OV);
    } else if (warning) {
        writeWord(VP_WARN_CHG_OV, WARN_ID_CHG_OV);
    } else {
        writeWord(VP_WARN_CHG_OV, WARN_ID_NORMAL);
    }
}

void BMSDwin::updateWarningChgOC(bool warning, bool alarm) {
    if (alarm) {
        writeWord(VP_WARN_CHG_OC, WARN_ID_CHG_OC);
    } else if (warning) {
        writeWord(VP_WARN_CHG_OC, WARN_ID_CHG_OC);
    } else {
        writeWord(VP_WARN_CHG_OC, WARN_ID_NORMAL);
    }
}

void BMSDwin::updateWarningChgTemp(bool warning, bool alarm) {
    if (alarm) {
        writeWord(VP_WARN_CHG_TEMP, WARN_ID_CHG_TEMP);
    } else if (warning) {
        writeWord(VP_WARN_CHG_TEMP, WARN_ID_CHG_TEMP);
    } else {
        writeWord(VP_WARN_CHG_TEMP, WARN_ID_NORMAL);
    }
}

void BMSDwin::updateWarningDsgUV(bool warning, bool alarm) {
    if (alarm) {
        writeWord(VP_WARN_DSG_UV, WARN_ID_DSG_UV);
    } else if (warning) {
        writeWord(VP_WARN_DSG_UV, WARN_ID_DSG_UV);
    } else {
        writeWord(VP_WARN_DSG_UV, WARN_ID_NORMAL);
    }
}

void BMSDwin::updateWarningDsgOC(bool warning, bool alarm) {
    if (alarm) {
        writeWord(VP_WARN_DSG_OC, WARN_ID_DSG_OC);
    } else if (warning) {
        writeWord(VP_WARN_DSG_OC, WARN_ID_DSG_OC);
    } else {
        writeWord(VP_WARN_DSG_OC, WARN_ID_NORMAL);
    }
}

void BMSDwin::updateWarningDsgTemp(bool warning, bool alarm) {
    if (alarm) {
        writeWord(VP_WARN_DSG_TEMP, WARN_ID_DSG_TEMP);
    } else if (warning) {
        writeWord(VP_WARN_DSG_TEMP, WARN_ID_DSG_TEMP);
    } else {
        writeWord(VP_WARN_DSG_TEMP, WARN_ID_NORMAL);
    }
}

void BMSDwin::updateAllWarnings(bool chg_ov_warn, bool chg_ov_alarm,
                      bool chg_oc_warn, bool chg_oc_alarm,
                      bool chg_temp_warn, bool chg_temp_alarm,
                      bool dsg_uv_warn, bool dsg_uv_alarm,
                      bool dsg_oc_warn, bool dsg_oc_alarm,
                      bool dsg_temp_warn, bool dsg_temp_alarm) {
    updateWarningChgOV(chg_ov_warn, chg_ov_alarm);
    updateWarningChgOC(chg_oc_warn, chg_oc_alarm);
    updateWarningChgTemp(chg_temp_warn, chg_temp_alarm);
    updateWarningDsgUV(dsg_uv_warn, dsg_uv_alarm);
    updateWarningDsgOC(dsg_oc_warn, dsg_oc_alarm);
    updateWarningDsgTemp(dsg_temp_warn, dsg_temp_alarm);
}

void BMSDwin::resetDisplay() {
    Serial.println("Resetting DWIN Display...");
    
    sendVoltages(0, 0, 0, 0, 0);
    sendCurrent(0);
    sendTemperature(0);
    
    writeWord(VP_WARN_CHG_OV, WARN_ID_NORMAL);
    writeWord(VP_WARN_CHG_OC, WARN_ID_NORMAL);
    writeWord(VP_WARN_CHG_TEMP, WARN_ID_NORMAL);
    writeWord(VP_WARN_DSG_UV, WARN_ID_NORMAL);
    writeWord(VP_WARN_DSG_OC, WARN_ID_NORMAL);
    writeWord(VP_WARN_DSG_TEMP, WARN_ID_NORMAL);
    
    writeWord(VP_ICON_CURRENT, ICON_IDLE);
    
    Serial.println("DWIN Display reset complete");
}

void BMSDwin::printDebug() {
    Serial.println("\n╔═══ DWIN STATUS ═══╗");
    Serial.println("DWIN Display:");
    Serial.println("   Serial2: TX=17, RX=16");
    Serial.println("   Baud: 115200");
    Serial.println("   Protocol: 0x5A 0xA5");
    Serial.println("╚═══════════════════╝\n");
}