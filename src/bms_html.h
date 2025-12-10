#ifndef BMS_HTML_H
#define BMS_HTML_H

#include <Arduino.h>
#include "bms_html_styles.h"
#include "bms_html_scripts.h"

/**
 * ═══════════════════════════════════════════════════════════
 *  BMS HTML DASHBOARD - Main Structure
 * ═══════════════════════════════════════════════════════════
 */

String getHTMLPage() {
    String html = F(R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 BMS Dashboard</title>
    <style>
)rawliteral");

    html += getHTMLStyles();
    
    html += F(R"rawliteral(
    </style>
</head>
<body>
    <div class="container">
        <div class="dashboard">
            <!-- HEADER -->
            <div class="header">
                <h1>🔋 BMS Dashboard</h1>
                <div class="header-center">
                    <div class="instructor-info">GVHD: PGS TS Nguyễn Chiến Trinh</div>
                    <div class="student-group">
                        <span class="sv-label">SV:</span>
                        <div class="student-list">
                            <div class="student-info">Hoàng Văn Quang_B21DCVT363</div>
                            <div class="student-info">Phạm Thành Long_B21DCVT275</div>
                        </div>
                    </div>
                </div>
                <div class="header-right">
                    <div class="status-badge" id="chargingStatus">
                        <span class="status-icon">⚡</span>
                        <span class="status-text">Idle</span>
                    </div>
                    <div class="live-indicator">
                        <span class="dot"></span>
                        <span>Live</span>
                    </div>
                </div>
            </div>

            <!-- ALERTS SECTION -->
            <div id="alertsContainer" class="alerts-container hidden">
                <!-- Alerts will be inserted here dynamically -->
            </div>

            <!-- STATS CARDS -->
            <div class="section">
                <h2>📊 Trạng Thái Pack</h2>
                <div class="stats-grid">
                    <div class="stat-card">
                        <div class="stat-icon">⚡</div>
                        <div class="stat-info">
                            <div class="stat-label">Pack Voltage</div>
                            <div class="stat-value" id="packVolt">--</div>
                        </div>
                    </div>
                    
                    <!-- SOC Card with Circular Progress -->
                    <div class="stat-card soc-card">
                        <div class="stat-info">
                            <div class="stat-label">State of Charge</div>
                        </div>
                        <div class="soc-circular-container">
                            <div class="circle" id="socCircle">
                                <div class="inner-circle">
                                    <span class="soc-percentage" id="socPercentage">--</span>
                                </div>
                            </div>
                        </div>
                    </div>
                    
                    <div class="stat-card">
                        <div class="stat-icon">❤️</div>
                        <div class="stat-info">
                            <div class="stat-label">State of Health</div>
                            <div class="stat-value" id="soh">--</div>
                        </div>
                    </div>
                    
                    <div class="stat-card">
                        <div class="stat-icon">⚡</div>
                        <div class="stat-info">
                            <div class="stat-label">Current</div>
                            <div class="stat-value" id="current">--</div>
                        </div>
                    </div>

                    <div class="stat-card">
                        <div class="stat-icon">🌡️</div>
                        <div class="stat-info">
                            <div class="stat-label">Pack Temperature</div>
                            <div class="stat-value" id="packTemp">--</div>
                        </div>
                    </div>

                    <div class="stat-card">
                        <div class="stat-icon">⚖️</div>
                        <div class="stat-info">
                            <div class="stat-label">Balancing</div>
                            <div class="stat-value small" id="balancingStatus">Inactive</div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- PROTECTION STATUS -->
            <div class="section">
                <h2>🛡️ Protection Status</h2>
                <div class="protection-grid">
                    <div class="protection-item" id="protectOV">
                        <div class="protection-icon">⚠️</div>
                        <div class="protection-info">
                            <div class="protection-label">Over Voltage</div>
                            <div class="protection-status">Normal</div>
                        </div>
                    </div>

                    <div class="protection-item" id="protectUV">
                        <div class="protection-icon">⚠️</div>
                        <div class="protection-info">
                            <div class="protection-label">Under Voltage</div>
                            <div class="protection-status">Normal</div>
                        </div>
                    </div>

                    <div class="protection-item" id="protectOC">
                        <div class="protection-icon">⚠️</div>
                        <div class="protection-info">
                            <div class="protection-label">Over Current</div>
                            <div class="protection-status">Normal</div>
                        </div>
                    </div>

                    <div class="protection-item" id="protectSC">
                        <div class="protection-icon">⚠️</div>
                        <div class="protection-info">
                            <div class="protection-label">Short Circuit</div>
                            <div class="protection-status">Normal</div>
                        </div>
                    </div>

                    <div class="protection-item" id="protectOT">
                        <div class="protection-icon">⚠️</div>
                        <div class="protection-info">
                            <div class="protection-label">Over Temperature</div>
                            <div class="protection-status">Normal</div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- BATTERY CELLS -->
            <div class="section">
                <h2>📦 Trạng Thái Các Cell</h2>
                <div id="batteryDisplay" class="battery-grid">
                    <div class="loading">Đang tải dữ liệu...</div>
                </div>
            </div>
        </div>
    </div>

    <script>
)rawliteral");

    html += getHTMLScripts();
    
    html += F(R"rawliteral(
    </script>
</body>
</html>
)rawliteral");

    return html;
}

#endif // BMS_HTML_H