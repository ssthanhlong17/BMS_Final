#ifndef BMS_PROTECTION_H
#define BMS_PROTECTION_H

#include <Arduino.h>

class BMSProtection {
private:
    // Cấu hình chân MOSFET
    const int PIN_CHG = 22;
    const int PIN_DSG = 23;
    
    // Ngưỡng bảo vệ - Charging
    const float CHG_OV_WARN = 3.45f;
    const float CHG_OV_TRIP = 3.65f;
    const float CHG_OV_REL  = 3.40f;
    const unsigned long CHG_OV_RECOVER_MS = 5000;
    
    const float CHG_OC_WARN = 1.0f;
    const float CHG_OC_TRIP = 1.4f;
    const float CHG_OC_REL  = 0.8f;
    const unsigned long CHG_OC_RECOVER_MS = 5000;
    
    const float CHG_OT_WARN = 40.0f;
    const float CHG_UT_WARN = 5.0f;
    const float CHG_OT_TRIP = 45.0f;
    const float CHG_OT_REL  = 38.0f;
    const float CHG_UT_TRIP = 0.0f;
    const float CHG_UT_REL  = 3.0f;
    const unsigned long CHG_TEMP_RECOVER_MS = 5000;
    
    // Ngưỡng bảo vệ - Discharging
    const float DSG_UV_WARN = 3.00f;
    const float DSG_UV_TRIP = 2.50f;
    const float DSG_UV_REL  = 2.90f;
    const unsigned long DSG_UV_RECOVER_MS = 5000;
    
    const float DSG_OC_WARN = -4.0f;
    const float DSG_OC_TRIP = -6.0f;
    const float DSG_OC_REL  = -3.5f;
    const unsigned long DSG_OC_RECOVER_MS = 5000;
    
    const float DSG_OT_WARN = 55.0f;
    const float DSG_UT_WARN = -5.0f;
    const float DSG_OT_TRIP = 60.0f;
    const float DSG_OT_REL  = 50.0f;
    const float DSG_UT_TRIP = -10.0f;
    const float DSG_UT_REL  = -8.0f;
    const unsigned long DSG_TEMP_RECOVER_MS = 5000;
    
    // Trạng thái bảo vệ
    bool chg_ov_fault;
    bool chg_oc_fault;
    bool chg_temp_fault;
    unsigned long chg_ov_recover_timer;
    unsigned long chg_oc_recover_timer;
    unsigned long chg_temp_recover_timer;
    
    bool dsg_uv_fault;
    bool dsg_oc_fault;
    bool dsg_temp_fault;
    unsigned long dsg_uv_recover_timer;
    unsigned long dsg_oc_recover_timer;
    unsigned long dsg_temp_recover_timer;
    
    // Hàm nội bộ
    bool checkChargeOV(float cell1, float cell2, float cell3, float cell4);
    bool checkChargeOC(float current);
    bool checkChargeTemp(float temp);
    bool checkDischargeUV(float cell1, float cell2, float cell3, float cell4);
    bool checkDischargeOC(float current);
    bool checkDischargeTemp(float temp);

public:
    BMSProtection();
    void begin();
    void update(float cell1, float cell2, float cell3, float cell4, 
                float current, float temp);
    
    // Getters
    bool isChargeFault() const;
    bool isDischargeFault() const;
    bool isAnyFault() const;
    bool getChargeMosfetState() const;
    bool getDischargeMosfetState() const;
    
    // Control
    void clearProtection();
    void printStatus();
};

#endif