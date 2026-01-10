#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ============ Include Modules ============
#include "soc_estimator.h"
#include "soh_estimator.h"
#include "bms_sensors.h"
#include "bms_protection.h"
#include "bms_balancing.h"
#include "bms_dwin.h"
#include "bms_data.h"      
#include "bms_html.h"

// ============ WiFi AP Configuration ============
const char* AP_SSID = "ESP32_BMS";
const char* AP_PASSWORD = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// ============ Web Server ============
WebServer server(80);

// ============ BMS Objects ============
BMSSensors sensors;
BMSProtection protection;
BMSBalancing balancing;
BMSDwin dwin;
SOCEstimator soc(6.0);
SOHEstimator soh(6.0);
// BMSTestMode testMode;  // Optional

// ============ Flags ============
bool socInitialized = false;
bool sohInitialized = false;

// ============ Timing ============
unsigned long lastBMSUpdate = 0;
unsigned long lastSOCUpdate = 0;
unsigned long lastSOHUpdate = 0;
unsigned long lastDebugPrint = 0;
unsigned long lastDwinUpdate = 0;

const unsigned long BMS_UPDATE_INTERVAL = 100;     // 100ms - Sensors + Protection + Balancing
const unsigned long SOC_UPDATE_INTERVAL = 1000;    // 1s
const unsigned long SOH_UPDATE_INTERVAL = 10000;   // 10s
const unsigned long DEBUG_PRINT_INTERVAL = 5000;   // 5s
const unsigned long DWIN_UPDATE_INTERVAL = 1000;   // 1s

// ============================================
// WIFI ACCESS POINT SETUP
// ============================================
void setupWiFiAP() {
    
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (apStarted) {
        Serial.println("WiFi Access Point started!");
        Serial.printf("SSID: %s\n", AP_SSID);
        Serial.printf("Password: %s\n", AP_PASSWORD);
        Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("Failed to start Access Point!");
    }
}

// ============================================
// WEB SERVER SETUP
// ============================================
void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", getHTMLPage());
    });
    
    server.on("/bms", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", getBMSJson());
    });
    
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not Found");
    });
    
    Serial.println("Web server routes configured");
}

// ============================================
// SERIAL COMMAND HANDLER
// ============================================
void handleSerialCommand() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        // ===== DEBUG COMMANDS =====
        if (cmd == "soh") {
            soh.printDebug();
        }
        else if (cmd == "soc") {
            soc.printDebug(bmsData.packVoltage, bmsData.current, bmsData.packTemp);
        }
        else if (cmd == "sensors") {
            sensors.printDebug();
        }
        else if (cmd == "protection") {
            protection.printStatus();
        }
        else if (cmd == "balance") {
            balancing.printStatus();
        }
        else if (cmd == "dwin") {
            dwin.printDebug();
        }
        else if (cmd == "data") {
            // Debug bmsData struct
            Serial.println("\n╔═══ BMS DATA STRUCT ═══╗");
            Serial.printf("Pack: %.3fV\n", bmsData.packVoltage);
            Serial.printf("Current: %+.3fA\n", bmsData.current);
            Serial.printf("Temp: %.1f°C\n", bmsData.packTemp);
            Serial.printf("SOC: %.1f%%\n", bmsData.soc);
            Serial.printf("SOH: %.1f%%\n", bmsData.soh);
            Serial.printf("Balancing: %s (Cell %d)\n", 
                         bmsData.balancingActive ? "YES" : "NO",
                         bmsData.balancingCell);
            Serial.printf("CHG MOSFET: %s\n", bmsData.chargeMosfetEnabled ? "ON" : "OFF");
            Serial.printf("DSG MOSFET: %s\n", bmsData.dischargeMosfetEnabled ? "ON" : "OFF");
            Serial.println("╚═══════════════════════╝\n");
        }
        
        // ===== CALIBRATION COMMANDS =====
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
                Serial.println("Invalid capacity (0-10Ah)");
            }
        }
        
        // ===== SYSTEM COMMANDS =====
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
            Serial.println("╚═══════════════════╝\n");
        }
        else if (cmd == "clients") {
            Serial.printf("\nConnected clients: %d / 4\n\n", WiFi.softAPgetStationNum());
        }
        // ===== HELP =====
        else if (cmd == "help") {
            Serial.println("\n╔═══════════════════ COMMANDS ═══════════════════╗");
            Serial.println("│ MONITORING:                                    │");
            Serial.println("│  soc         - SOC debug info                  │");
            Serial.println("│  soh         - SOH debug info                  │");
            Serial.println("│  sensors     - Sensor readings                 │");
            Serial.println("│  protection  - Protection status               │");
            Serial.println("│  balance     - Balancing status                │");
            Serial.println("│  dwin        - DWIN display info               │");
            Serial.println("│  data        - BMS Data struct                 │");
            Serial.println("│  json        - JSON API output                 │");
            Serial.println("│                                                │");
            Serial.println("│ CALIBRATION:                                   │");
            Serial.println("│  reset_soh   - Reset SOH to 100%               │");
            Serial.println("│  reset_cycles- Reset cycle counter             │");
            Serial.println("│  cal_soh X.X - Calibrate SOH (Ah)              │");
            Serial.println("│                                                │");
            Serial.println("│ SYSTEM:                                        │");             
            Serial.println("│  help        - Show this menu                  │");
            Serial.println("╚════════════════════════════════════════════════╝\n");
        }
        else if (cmd != "") {
            Serial.println("Unknown command. Type 'help' for list");
        }
    }
}

// ============================================
// PRINT BMS STATUS
// ============================================
void printBMSStatus() {
    // Time & Info
    Serial.printf("  %lus |  %.1f°C | %d clients\n", 
                  millis()/1000, bmsData.packTemp, WiFi.softAPgetStationNum());
    
    // Cells
    Serial.println("CELLS:");
    for (int i = 0; i < NUM_CELLS; i++) {
        Serial.printf("   Cell %d: %.3fV", i+1, bmsData.cellVoltages[i]);
        if (bmsData.balancingCells[i]) {
            Serial.print(" [ BALANCING]");
        }
        Serial.println();
    }
    
    // Pack
    Serial.println("\nPACK:");
    Serial.printf("   Voltage: %.3fV\n", bmsData.packVoltage);
    Serial.printf("   Current: %+.3fA", bmsData.current);
    if (bmsData.isCharging) Serial.print(" CHG");
    else if (bmsData.isDischarging) Serial.print(" DSG");
    else Serial.print(" IDLE");
    Serial.println();
    
    // SOC & SOH
    Serial.println("\nSTATE:");
    Serial.printf("   SOC: %.1f%%\n", bmsData.soc);
    Serial.printf("   SOH: %.1f%% (%.2fAh)\n", bmsData.soh, bmsData.remainingCapacity);
    Serial.printf("   Cycles: %.1f / %.0f remaining\n", 
                  bmsData.totalCycles, bmsData.remainingCycles);
    
    // Protection
    Serial.println("\n PROTECTION:");
    Serial.printf("   CHG: %s | DSG: %s", 
                  bmsData.chargeMosfetEnabled ? "Yes" : "No",
                  bmsData.dischargeMosfetEnabled ? "Yes" : "No");
    
    bool hasAlarm = bmsData.overVoltageAlarm || bmsData.underVoltageAlarm ||
                    bmsData.overCurrentChargeAlarm || bmsData.overCurrentDischargeAlarm ||
                    bmsData.overTempChargeAlarm || bmsData.overTempDischargeAlarm;
    
    if (hasAlarm) {
        Serial.print(" | FAULT");
    }
    Serial.println();
      
    // Balancing
    Serial.println("\n BALANCING:");
    Serial.printf("   Status: %s\n", bmsData.balancingActive ? "ACTIVE" : "INACTIVE");
    if (bmsData.balancingActive) {
        Serial.printf("   Cell: %d\n", bmsData.balancingCell);
    }
    
    Serial.printf("Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
}

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n");
    
    // Initialize BMS Data
    initBMSData();
    Serial.println("BMS Data initialized");
    
    // Initialize Modules
    sensors.begin();
    protection.begin();
    balancing.begin();
    dwin.begin();
    soh.begin();
    sohInitialized = true;
    
    // Setup WiFi & Web
    setupWiFiAP();
    setupWebServer();
    server.begin();
    Serial.printf("Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("Type 'help' for commands");
}

// ============================================

void loop() {
    unsigned long now = millis();
    
    // Handle web requests
    server.handleClient();
    
    // Handle Serial commands
    handleSerialCommand();
    
    // =====  CẬP NHẬT TOÀN BỘ BMS (100ms) =====
    if (now - lastBMSUpdate >= BMS_UPDATE_INTERVAL) {
        lastBMSUpdate = now;
        updateAllBMSData();
    }
    
    // ===== CẬP NHẬT SOC (1s) =====
    if (now - lastSOCUpdate >= SOC_UPDATE_INTERVAL) {
        lastSOCUpdate = now;
        updateSOC();  // Hàm trong bms_data.h
    }
    
    // ===== CẬP NHẬT SOH (10s) =====
    if (now - lastSOHUpdate >= SOH_UPDATE_INTERVAL) {
        lastSOHUpdate = now;
        updateSOH();  // Hàm trong bms_data.h
    }
    
    // =====  CẬP NHẬT DWIN (1s) =====
    if (now - lastDwinUpdate >= DWIN_UPDATE_INTERVAL) {
        lastDwinUpdate = now;
        updateDWINDisplay();  // Hàm trong bms_data.h
    }
    
    // ===== IN DEBUG (5s) =====
    if (now - lastDebugPrint >= DEBUG_PRINT_INTERVAL) {
        lastDebugPrint = now;
        printBMSStatus();  // Hàm trong main.cpp
    }
    
    delay(10);
}