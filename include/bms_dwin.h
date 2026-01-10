#ifndef BMS_DWIN_H
#define BMS_DWIN_H

#include <Arduino.h>

class BMSDwin {
private:
    // Địa chỉ VP (Variable Pointer)
    const uint16_t VP_CELL1 = 0x2000;
    const uint16_t VP_CELL2 = 0x2100;
    const uint16_t VP_CELL3 = 0x2200;
    const uint16_t VP_CELL4 = 0x2300;
    const uint16_t VP_PACK  = 0x1000;
    
    const uint16_t VP_CURRENT = 0x1200;
    const uint16_t VP_TEMP    = 0x1300;
    const uint16_t VP_ICON_CURRENT = 0x1400;
    
    // Cảnh báo bảo vệ
    const uint16_t VP_WARN_CHG_OV   = 0x1510;
    const uint16_t VP_WARN_CHG_OC   = 0x1520;
    const uint16_t VP_WARN_CHG_TEMP = 0x1530;
    const uint16_t VP_WARN_DSG_UV   = 0x1540;
    const uint16_t VP_WARN_DSG_OC   = 0x1550;
    const uint16_t VP_WARN_DSG_TEMP = 0x1560;
    
    // Icon IDs
    const uint16_t ICON_CHARGING    = 0;
    const uint16_t ICON_DISCHARGING = 1;
    const uint16_t ICON_IDLE        = 2;
    
    // Warning IDs
    const uint16_t WARN_ID_CHG_OV   = 5;
    const uint16_t WARN_ID_CHG_OC   = 6;
    const uint16_t WARN_ID_CHG_TEMP = 7;
    const uint16_t WARN_ID_DSG_UV   = 8;
    const uint16_t WARN_ID_DSG_OC   = 9;
    const uint16_t WARN_ID_DSG_TEMP = 10;
    const uint16_t WARN_ID_NORMAL   = 11;
    
    // Hàm nội bộ
    void writeWord(uint16_t vp, int16_t value);
    void writeFloat(uint16_t vp, float value, uint8_t decimal_places);

public:
    BMSDwin();
    void begin();
    
    // Gửi dữ liệu
    void sendVoltages(float cell1, float cell2, float cell3, float cell4, float pack);
    void sendCurrent(float current);
    void sendCurrentIcon(float current);
    void sendTemperature(float temp);
    void updateBasicData(float cell1, float cell2, float cell3, float cell4,
                        float pack, float current, float temp);
    
    // Cập nhật cảnh báo
    void updateWarningChgOV(bool warning, bool alarm);
    void updateWarningChgOC(bool warning, bool alarm);
    void updateWarningChgTemp(bool warning, bool alarm);
    void updateWarningDsgUV(bool warning, bool alarm);
    void updateWarningDsgOC(bool warning, bool alarm);
    void updateWarningDsgTemp(bool warning, bool alarm);
    void updateAllWarnings(bool chg_ov_warn, bool chg_ov_alarm,
                          bool chg_oc_warn, bool chg_oc_alarm,
                          bool chg_temp_warn, bool chg_temp_alarm,
                          bool dsg_uv_warn, bool dsg_uv_alarm,
                          bool dsg_oc_warn, bool dsg_oc_alarm,
                          bool dsg_temp_warn, bool dsg_temp_alarm);
    
    // Control
    void resetDisplay();
    void printDebug();
};

#endif