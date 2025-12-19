#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>


#include "soc_estimator.h"
#include "soh_estimator.h"
#include "bms_sensors.h"
#include "bms_data.h"
#include "bms_html.h"

// ============ WiFi AP Configuration ============
const char* AP_SSID = "ESP32_BMS";           // Tên WiFi phát ra
const char* AP_PASSWORD = "12345678";        // Mật khẩu (tối thiểu 8 ký tự)
const IPAddress AP_IP(192, 168, 4, 1);       // IP của ESP32
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// ============ Web Server ============
WebServer server(80);

// ============ BMS Objects ============
BMSSensors sensors;
SOCEstimator soc(6.0);  // 6Ah battery
SOHEstimator soh(6.0);  // 6Ah nominal capacity

// ============ Flags ============
bool socInitialized = false;
bool sohInitialized = false;

// ============ MOSFET Protection Pins ============
const int PIN_CHG = 22;
const int PIN_DSG = 23;

// ============ Protection Thresholds ============
const float CELL_OVERVOLTAGE = 3.65f;
const float CELL_UNDERVOLTAGE = 2.50f;
const float I_WARNLIMIT = 0.7f;
const float I_PROTECTLIMIT = 0.9f;

bool protectionTriggered = false;

// ============ Timing ============
unsigned long lastSensorRead = 0;
unsigned long lastSOCUpdate = 0;
unsigned long lastSOHUpdate = 0;
unsigned long lastDebugPrint = 0;

const unsigned long SENSOR_READ_INTERVAL = 500;   // 500ms
const unsigned long SOC_UPDATE_INTERVAL = 1000;   // 1s
const unsigned long SOH_UPDATE_INTERVAL = 10000;  // 10s
const unsigned long DEBUG_PRINT_INTERVAL = 2000;  // 5s

// ============================================
// WIFI ACCESS POINT SETUP
// ============================================
void setupWiFiAP() {
    Serial.println("\n📡 Setting up WiFi Access Point...");
    
    // Tắt WiFi trước khi cấu hình
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Cấu hình IP tĩnh
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    
    // Khởi động Access Point
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (apStarted) {
        Serial.println("✅ WiFi Access Point started!");
        Serial.println("════════════════════════════════════════");
        Serial.print("📶 SSID: ");
        Serial.println(AP_SSID);
        Serial.print("🔐 Password: ");
        Serial.println(AP_PASSWORD);
        Serial.print("📍 IP Address: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("👥 Max Clients: ");
        Serial.println("4");  // ESP32 mặc định hỗ trợ 4 client
        Serial.println("════════════════════════════════════════");
        Serial.println("💡 Connect your device to this WiFi");
        Serial.print("🌐 Then open: http://");
        Serial.println(WiFi.softAPIP());
        Serial.println("════════════════════════════════════════");
    } else {
        Serial.println("❌ Failed to start Access Point!");
    }
}

// ============================================
// WEB SERVER SETUP
// ============================================
void setupWebServer() {
    // Root endpoint - HTML Dashboard
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", getHTMLPage());
    });
    
    // BMS data JSON API
    server.on("/bms", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", getBMSJson());
    });
    
    // 404 handler
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not Found");
    });
    
    Serial.println("✅ Web server routes configured");
}

// ============================================
// SERIAL COMMAND HANDLER
// ============================================
void handleSerialCommand() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "soh") {
            soh.printDebug();
        }
        else if (cmd == "soc") {
            soc.printDebug(sensors.getPackVoltage(), sensors.getCurrent(), sensors.getTemperature());
        }
        else if (cmd == "sensors") {
            sensors.printDebug();
        }
        else if (cmd == "reset_soh") {
            soh.resetSOH();
        }
        else if (cmd == "reset_cycles") {
            soh.resetCycles();
        }
        else if (cmd.startsWith("cal_soh ")) {
            float capacity = cmd.substring(8).toFloat();
            if (capacity > 0 && capacity <= 10.0) {
                soh.calibrateFromCapacity(capacity);
            } else {
                Serial.println("❌ Invalid capacity (0-10Ah)");
            }
        }
        else if (cmd == "json") {
            Serial.println(getBMSJson());
        }
        else if (cmd == "wifi") {
            Serial.println("\n╔═══ WiFi AP INFO ═══╗");
            Serial.printf("│ Mode: Access Point\n");
            Serial.printf("│ SSID: %s\n", AP_SSID);
            Serial.printf("│ Password: %s\n", AP_PASSWORD);
            Serial.printf("│ IP: %s\n", WiFi.softAPIP().toString().c_str());
            Serial.printf("│ Clients: %d\n", WiFi.softAPgetStationNum());
            Serial.printf("│ Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
            Serial.println("╚════════════════════╝\n");
        }
        else if (cmd == "clients") {
            Serial.println("\n╔═══ CONNECTED CLIENTS ═══╗");
            Serial.printf("│ Total: %d / 4\n", WiFi.softAPgetStationNum());
            Serial.println("╚═════════════════════════╝\n");
        }
        else if (cmd == "restart") {
            Serial.println("🔄 Restarting ESP32...");
            delay(1000);
            ESP.restart();
        }
        else if (cmd == "protection") {
            Serial.println("\n╔═══ PROTECTION STATUS ═══╗");
            Serial.printf("│ CHG MOSFET: %s\n", digitalRead(PIN_CHG) ? "ON ✅" : "OFF 🔴");
            Serial.printf("│ DSG MOSFET: %s\n", digitalRead(PIN_DSG) ? "ON ✅" : "OFF 🔴");
            Serial.println("├─────────────────────────┤");
            Serial.println("│ CHARGING PROTECTION:    │");
            Serial.printf("│  - Over Voltage: %s\n", bmsData.overVoltageAlarm ? "ALARM 🔴" : "OK ✅");
            Serial.printf("│  - Over Current: %s\n", bmsData.overCurrentChargeAlarm ? "ALARM 🔴" : "OK ✅");
            Serial.println("├─────────────────────────┤");
            Serial.println("│ DISCHARGING PROTECTION: │");
            Serial.printf("│  - Under Voltage: %s\n", bmsData.underVoltageAlarm ? "ALARM 🔴" : "OK ✅");
            Serial.printf("│  - Over Current: %s\n", bmsData.overCurrentDischargeAlarm ? "ALARM 🔴" : "OK ✅");
            Serial.println("├─────────────────────────┤");
            Serial.printf("│ Protection Status: %s\n", protectionTriggered ? "TRIGGERED ⚠️" : "NORMAL ✅");
            Serial.println("╚═════════════════════════╝\n");
        }
        else if (cmd == "clear") {
            // Clear protection (manual override)
            if (protectionTriggered) {
                Serial.println("⚠️  Manually clearing protection...");
                digitalWrite(PIN_CHG, HIGH);
                digitalWrite(PIN_DSG, HIGH);
                protectionTriggered = false;
                Serial.println("✅ Protection cleared (use with caution!)");
            } else {
                Serial.println("✅ No protection to clear");
            }
        }
        else if (cmd == "balance") {
            Serial.println("\n╔═══ BALANCE INFO ═══╗");
            Serial.printf("│ Active: %s\n", bmsData.balancingActive ? "YES ⚖️" : "NO");
            if (bmsData.balancingActive) {
                Serial.print("│ Cells: ");
                for (int i = 0; i < NUM_CELLS; i++) {
                    if (bmsData.balancingCells[i]) {
                        Serial.printf("C%d ", i+1);
                    }
                }
                Serial.println();
            }
            float imbalance = sensors.getCellImbalance();
            Serial.printf("│ Imbalance: %.3fV\n", imbalance);
            Serial.println("╚════════════════════╝\n");
        }
        else if (cmd == "help") {
            Serial.println("\n╔══════════════ COMMANDS ══════════════╗");
            Serial.println("│ MONITORING:                          │");
            Serial.println("│  soc         - SOC debug info        │");
            Serial.println("│  soh         - SOH debug info        │");
            Serial.println("│  sensors     - Sensor readings       │");
            Serial.println("│  json        - JSON API output       │");
            Serial.println("│  protection  - Protection status     │");
            Serial.println("│  balance     - Balancing info        │");
            Serial.println("│                                      │");
            Serial.println("│ CALIBRATION:                         │");
            Serial.println("│  reset_soh   - Reset SOH to 100%     │");
            Serial.println("│  reset_cycles- Reset cycle counter   │");
            Serial.println("│  cal_soh X.X - Calibrate SOH (Ah)    │");
            Serial.println("│                                      │");
            Serial.println("│ SYSTEM:                              │");
            Serial.println("│  wifi        - WiFi AP information   │");
            Serial.println("│  clients     - Connected clients     │");
            Serial.println("│  restart     - Restart ESP32         │");
            Serial.println("│  clear       - Clear protection      │");
            Serial.println("│  help        - Show this menu        │");
            Serial.println("╚══════════════════════════════════════╝\n");
        }
        else if (cmd != "") {
            Serial.println("❌ Unknown command. Type 'help' for list");
        }
    }
}

// ============================================
// BMS READ AND UPDATE
// ============================================
void readAndUpdateBMS() {
    // Đọc sensors
    sensors.readAllSensors();
    
    float cell1 = sensors.getCellVoltage(1);
    float cell2 = sensors.getCellVoltage(2);
    float cell3 = sensors.getCellVoltage(3);
    float cell4 = sensors.getCellVoltage(4);
    float current = sensors.getCurrent();
    float temp = sensors.getTemperature();
    float packVoltage = sensors.getPackVoltage();
    
    // Khởi tạo SOC lần đầu
    if (!socInitialized) {
        soc.initializeFromVoltage(packVoltage);
        socInitialized = true;
        Serial.printf("🔋 SOC initialized: %.1f%%\n", soc.getSOC());
    }
    
    // Cập nhật BMS data structure (cho JSON API)
    updateBMSData(cell1, cell2, cell3, cell4, current, temp);
}

// ============================================
// PROTECTION SYSTEM
// ============================================
void handleProtection() {
    float cell1 = sensors.getCellVoltage(1);
    float cell2 = sensors.getCellVoltage(2);
    float cell3 = sensors.getCellVoltage(3);
    float cell4 = sensors.getCellVoltage(4);
    float current = sensors.getCurrent();
    
    // Over/Under Voltage
    bool overVoltage = (cell1 > CELL_OVERVOLTAGE || cell2 > CELL_OVERVOLTAGE || 
                        cell3 > CELL_OVERVOLTAGE || cell4 > CELL_OVERVOLTAGE);
    bool underVoltage = (cell1 < CELL_UNDERVOLTAGE || cell2 < CELL_UNDERVOLTAGE || 
                         cell3 < CELL_UNDERVOLTAGE || cell4 < CELL_UNDERVOLTAGE);
    
    if (overVoltage) {
        digitalWrite(PIN_CHG, LOW);
        if (!protectionTriggered) {
            Serial.println("🔴 OVERVOLTAGE PROTECTION!");
        }
        protectionTriggered = true;
    }
    
    if (underVoltage) {
        digitalWrite(PIN_DSG, LOW);
        if (!protectionTriggered) {
            Serial.println("🔴 UNDERVOLTAGE PROTECTION!");
        }
        protectionTriggered = true;
    }
    
    // Over Current Charging
    if (current >= I_PROTECTLIMIT) {
        digitalWrite(PIN_CHG, LOW);
        if (!protectionTriggered) {
            Serial.printf("🔴 CHG OVERCURRENT: %.2fA\n", current);
        }
        protectionTriggered = true;
    } else if (current < 0.4f && !underVoltage && !overVoltage) {
        digitalWrite(PIN_CHG, HIGH);
    }
    
    // Over Current Discharging
    if (current <= -I_PROTECTLIMIT) {
        digitalWrite(PIN_DSG, LOW);
        if (!protectionTriggered) {
            Serial.printf("🔴 DSG OVERCURRENT: %.2fA\n", current);
        }
        protectionTriggered = true;
    } else if (current > -0.4f && !underVoltage && !overVoltage) {
        digitalWrite(PIN_DSG, HIGH);
    }
    
    // Clear protection
    if (current > -I_WARNLIMIT && current < I_WARNLIMIT && 
        !overVoltage && !underVoltage) {
        if (protectionTriggered) {
            Serial.println("✅ Protection cleared\n");
        }
        protectionTriggered = false;
    }
}

// ============================================
// PRINT STATUS
// ============================================
void printBMSStatus() {
    Serial.println("\n════════════════════════════════════════");
    Serial.println("📊 BMS STATUS REPORT");
    Serial.println("════════════════════════════════════════");
    
    // Time & Temperature
    Serial.printf("⏱  %lus | 🌡 %.1f°C | 👥 %d clients\n", 
                  millis()/1000, sensors.getTemperature(), WiFi.softAPgetStationNum());
    Serial.println("────────────────────────────────────────");
    
    // Cells
    Serial.println("📦 CELLS:");
    for (int i = 1; i <= 4; i++) {
        Serial.printf("   Cell %d: %.3fV", i, sensors.getCellVoltage(i));
        if (bmsData.balancingCells[i-1]) {
            Serial.print(" [⚖️ BALANCING]");
        }
        Serial.println();
    }
    
    // Pack
    Serial.println("\n⚡ PACK:");
    Serial.printf("   Voltage: %.3fV\n", sensors.getPackVoltage());
    Serial.printf("   Current: %+.3fA", sensors.getCurrent());
    if (sensors.isCharging()) Serial.print(" ↑CHG");
    else if (sensors.isDischarging()) Serial.print(" ↓DSG");
    else Serial.print(" ⏸IDLE");
    Serial.println();
    
    // SOC & SOH
    Serial.println("\n📊 STATE:");
    Serial.printf("   SOC: %.1f%%\n", soc.getSOC());
    Serial.printf("   SOH: %.1f%% (%.2fAh)\n", soh.getSOH(), soh.getCurrentCapacity());
    Serial.printf("   Cycles: %.1f / %.0f remaining\n", 
                  soh.getTotalCycles(), soh.getRemainingCycles());
    
    // Protection
    Serial.println("\n🛡️  PROTECTION:");
    Serial.printf("   CHG: %s | DSG: %s", 
                  digitalRead(PIN_CHG) ? "✅" : "🔴",
                  digitalRead(PIN_DSG) ? "✅" : "🔴");
    if (protectionTriggered) Serial.print(" | ⚠ TRIGGERED");
    Serial.println();
    
    // Alarms
    if (bmsData.overVoltageAlarm) Serial.println("   🔴 Over Voltage ALARM (Charging)");
    if (bmsData.underVoltageAlarm) Serial.println("   🔴 Under Voltage ALARM (Discharging)");
    if (bmsData.overCurrentChargeAlarm) Serial.println("   🔴 Over Current ALARM (Charging)");
    if (bmsData.overCurrentDischargeAlarm) Serial.println("   🔴 Over Current ALARM (Discharging)");
        
    // SOH Warning
    if (soh.getSOH() < 90.0f) {
        Serial.printf("   ⚠️  SOH degraded: %.0f%%\n", soh.getSOH());
    }
    if (soh.getSOH() < 80.0f) {
        Serial.println("   🚨 Battery End of Life!");
    }
    
    Serial.println("════════════════════════════════════════");
    Serial.printf("📡 Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("════════════════════════════════════════\n");
}

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n");
    Serial.println("════════════════════════════════════════");
    Serial.println("🔋 ESP32 BMS System v2.2 - AP Mode");
    Serial.println("════════════════════════════════════════");
    
    // Initialize BMS Data
    initBMSData();
    Serial.println("✅ BMS Data initialized");
    
    // Initialize Sensors
    sensors.begin();
    
    // Initialize MOSFET pins
    pinMode(PIN_CHG, OUTPUT);
    pinMode(PIN_DSG, OUTPUT);
    digitalWrite(PIN_CHG, HIGH);
    digitalWrite(PIN_DSG, HIGH);
    Serial.println("✅ MOSFET protection pins initialized");
    
    // Initialize SOH
    soh.begin();
    sohInitialized = true;
    Serial.println("✅ SOH estimator initialized");
    
    // Setup WiFi Access Point
    setupWiFiAP();
    
    // Setup Web Server
    setupWebServer();
    server.begin();
    Serial.println("✅ HTTP server started");
    
    Serial.println("════════════════════════════════════════");
    Serial.println("🎉 BMS System Ready!");
    Serial.println("════════════════════════════════════════");
    Serial.printf("📡 Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("💡 Type 'help' for commands");
    Serial.println("════════════════════════════════════════\n");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
    unsigned long now = millis();
    
    // Handle web requests
    server.handleClient();
    
    // Handle Serial commands
    handleSerialCommand();
    
    // Read sensors
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = now;
        readAndUpdateBMS();
        handleProtection();
    }
    
    // Update SOC
    if (now - lastSOCUpdate >= SOC_UPDATE_INTERVAL) {
        lastSOCUpdate = now;
        if (socInitialized) {
            soc.update(sensors.getCurrent(), sensors.getTemperature());
            soc.recalibrate(sensors.getPackVoltage(), sensors.getCurrent());
        }
    }
    
    // Update SOH
    if (now - lastSOHUpdate >= SOH_UPDATE_INTERVAL) {
        lastSOHUpdate = now;
        if (sohInitialized && socInitialized) {
            soh.update(soc.getSOC(), sensors.getTemperature());
        }
    }
    
    // Print debug status
    if (now - lastDebugPrint >= DEBUG_PRINT_INTERVAL) {
        lastDebugPrint = now;
        printBMSStatus();
    }
    
    delay(10);  // Small delay for stability
}




  



