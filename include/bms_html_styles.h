#ifndef BMS_HTML_STYLES_H
#define BMS_HTML_STYLES_H

#include <Arduino.h>

/**
 * ═══════════════════════════════════════════════════════════
 *  BMS HTML STYLES - CSS
 * ═══════════════════════════════════════════════════════════
 */

String getHTMLStyles() {
    return F(R"rawliteral(
* { 
    margin: 0; 
    padding: 0; 
    box-sizing: border-box; 
}

body {
    font-family: 'Segoe UI', system-ui, sans-serif;
    background: linear-gradient(135deg, #e8eaf6 0%, #f5f5f5 100%);
    min-height: 100vh;
    padding: 20px;
}

.container {
    max-width: 1400px;
    margin: 0 auto;
    animation: fadeIn 0.6s ease;
}

@keyframes fadeIn {
    from { opacity: 0; transform: translateY(30px); }
    to { opacity: 1; transform: translateY(0); }
}

.dashboard {
    background: rgba(255, 255, 255, 0.95);
    backdrop-filter: blur(10px);
    border-radius: 24px;
    padding: 30px;
    box-shadow: 0 25px 80px rgba(0,0,0,0.15);
}

.header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
    padding-bottom: 20px;
    border-bottom: 2px solid #e0e0e0;
}

.header-center {
    display: flex;
    flex-direction: column;
    gap: 8px;
    align-items: center;
    flex: 1;
    margin: 0 20px;
}

.instructor-info {
    font-size: 1em;
    font-weight: 700;
    color: #424242;
    background: linear-gradient(135deg, #e8f5e9 0%, #c8e6c9 100%);
    padding: 8px 20px;
    border-radius: 20px;
    white-space: nowrap;
}

.student-group {
    display: flex;
    align-items: center;
    gap: 10px;
}

.sv-label {
    font-size: 0.95em;
    font-weight: 700;
    color: #424242;
}

.student-list {
    display: flex;
    gap: 10px;
}

.student-info {
    font-size: 0.9em;
    font-weight: 600;
    color: #424242;
    background: linear-gradient(135deg, #ffffff 0%, #f5f5f5 100%);
    padding: 6px 14px;
    border-radius: 20px;
    white-space: nowrap;
    border: 1px solid #e0e0e0;
}

.header-right {
    display: flex;
    gap: 15px;
    align-items: center;
}

h1 {
    background: linear-gradient(135deg, #00e676, #00c853);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    font-size: 2.2em;
    font-weight: 700;
    margin: 0;
}

.status-badge {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 16px;
    border-radius: 50px;
    font-weight: 600;
    transition: all 0.3s ease;
}

.status-badge.charging {
    background: #e8f5e9;
    color: #2e7d32;
}

.status-badge.discharging {
    background: #fff3e0;
    color: #f57c00;
}

.status-badge.idle {
    background: #f5f5f5;
    color: #757575;
}

.status-icon { font-size: 1.2em; }
.status-text { font-size: 0.95em; }

.live-indicator {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 16px;
    background: #e8f5e9;
    border-radius: 50px;
    font-weight: 600;
    color: #2e7d32;
}

.dot {
    width: 10px;
    height: 10px;
    background: #00e676;
    border-radius: 50%;
    animation: pulse 2s infinite;
}

@keyframes pulse {
    0%, 100% { box-shadow: 0 0 0 0 rgba(0, 230, 118, 0.7); }
    50% { box-shadow: 0 0 0 8px rgba(0, 230, 118, 0); }
}

.alerts-container {
    margin-bottom: 20px;
    display: flex;
    flex-direction: column;
    gap: 10px;
}

.alerts-container.hidden { display: none; }

.alert {
    display: flex;
    align-items: center;
    gap: 15px;
    padding: 15px 20px;
    border-radius: 12px;
    font-weight: 600;
    animation: slideIn 0.3s ease;
}

@keyframes slideIn {
    from { transform: translateX(-100%); opacity: 0; }
    to { transform: translateX(0); opacity: 1; }
}

.alert.critical {
    background: #ffebee;
    border-left: 5px solid #f44336;
    color: #c62828;
}

.alert.warning {
    background: #fff3e0;
    border-left: 5px solid #ff9800;
    color: #e65100;
}

.alert-icon { font-size: 1.5em; }

.section { margin-top: 30px; }
.section:first-of-type { margin-top: 0; }

h2 {
    color: #424242;
    font-size: 1.4em;
    font-weight: 600;
    margin-bottom: 15px;
}

.stats-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    grid-template-rows: repeat(3, auto);
    gap: 15px;
}

.stats-grid .stat-card:nth-child(1) { grid-column: 1; grid-row: 1; }
.stats-grid .stat-card:nth-child(2) { grid-column: 2; grid-row: 1 / 3; }
.stats-grid .stat-card:nth-child(3) { grid-column: 2; grid-row: 3; }
.stats-grid .stat-card:nth-child(4) { grid-column: 1; grid-row: 2; }
.stats-grid .stat-card:nth-child(5) { grid-column: 3; grid-row: 1; }
.stats-grid .stat-card:nth-child(6) { grid-column: 3; grid-row: 2; }

.stat-card {
    display: flex;
    align-items: center;
    gap: 15px;
    background: #ffffff;
    padding: 18px;
    border-radius: 16px;
    box-shadow: 0 4px 15px rgba(0,0,0,0.08);
    transition: all 0.3s ease;
    border: 2px solid #f5f5f5;
}

.stat-card:hover {
    transform: translateY(-5px);
    box-shadow: 0 8px 25px rgba(0, 230, 118, 0.2);
    border-color: #00e676;
}

.stat-card.soc-card {
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 25px 20px;
    min-height: 280px;
}

.stat-card.soc-card .stat-info {
    width: 100%;
    text-align: center;
    margin-bottom: 15px;
}

.soc-circular-container {
    display: flex;
    justify-content: center;
    align-items: center;
    margin: 10px 0;
}

.circle {
    width: 180px;
    height: 180px;
    border-radius: 50%;
    background: conic-gradient(#e0e0e0 0deg, #e0e0e0 360deg);
    display: flex;
    justify-content: center;
    align-items: center;
    transition: background 0.8s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 4px 15px rgba(0,0,0,0.1);
}

.inner-circle {
    width: 130px;
    height: 130px;
    border-radius: 50%;
    background: #fff;
    display: flex;
    justify-content: center;
    align-items: center;
    box-shadow: 0 0 10px rgba(0,0,0,0.1);
}

.soc-percentage {
    font-size: 2.5em;
    font-weight: 700;
    color: #212121;
    transition: opacity 0.3s ease;
}

.stat-icon {
    font-size: 2.2em;
    filter: drop-shadow(0 2px 4px rgba(0,0,0,0.1));
}

.stat-info { flex: 1; }

.stat-label {
    font-size: 0.85em;
    color: #757575;
    font-weight: 600;
    margin-bottom: 5px;
}

.stat-value {
    font-size: 1.6em;
    font-weight: 700;
    background: linear-gradient(135deg, #00e676, #00c853);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    font-family: 'Courier New', monospace;
    transition: opacity 0.3s ease;
}

.stat-value.small { font-size: 1.2em; }

.protection-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: 15px;
}

.protection-item {
    display: flex;
    align-items: center;
    gap: 12px;
    background: #ffffff;
    padding: 15px;
    border-radius: 12px;
    border-left: 4px solid #00e676;
    box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    transition: all 0.3s ease;
}

.protection-item.alert {
    border-left-color: #ff9800;
    background: #fff3e0;
}

.protection-item.alarm {
    border-left-color: #f44336;
    background: #ffebee;
    animation: shake 0.5s;
}

@keyframes shake {
    0%, 100% { transform: translateX(0); }
    25% { transform: translateX(-5px); }
    75% { transform: translateX(5px); }
}

.protection-icon { font-size: 1.8em; }

.protection-item.alert .protection-icon,
.protection-item.alarm .protection-icon {
    animation: bounce 1s infinite;
}

@keyframes bounce {
    0%, 100% { transform: translateY(0); }
    50% { transform: translateY(-5px); }
}

.protection-info { flex: 1; }

.protection-label {
    font-size: 0.9em;
    color: #424242;
    font-weight: 600;
    margin-bottom: 3px;
}

.protection-status {
    font-size: 0.85em;
    font-weight: 700;
    color: #00e676;
}

.protection-item.alert .protection-status { color: #f57c00; }
.protection-item.alarm .protection-status { color: #d32f2f; }

.battery-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: 15px;
    transition: opacity 0.3s ease;
}

.loading {
    grid-column: 1 / -1;
    text-align: center;
    padding: 40px;
    color: #757575;
    font-size: 1.1em;
    animation: fadeInOut 1.5s infinite;
}

@keyframes fadeInOut {
    0%, 100% { opacity: 0.4; }
    50% { opacity: 1; }
}

.battery-cell {
    background: #ffffff;
    border-radius: 16px;
    padding: 20px;
    text-align: center;
    box-shadow: 0 4px 15px rgba(0,0,0,0.08);
    transition: all 0.3s ease;
    border: 2px solid #f5f5f5;
    position: relative;
}

.battery-cell:hover {
    transform: translateY(-5px);
    box-shadow: 0 8px 20px rgba(0, 230, 118, 0.2);
    border-color: #00e676;
}

.battery-cell.balancing {
    border-color: #ff9800;
    box-shadow: 0 4px 15px rgba(255, 152, 0, 0.3);
}

.battery-cell.balancing::before {
    content: "⚖️";
    position: absolute;
    top: 8px;
    right: 8px;
    font-size: 1.1em;
    animation: spin 2s linear infinite;
}

@keyframes spin {
    from { transform: rotate(0deg); }
    to { transform: rotate(360deg); }
}

.cell-label {
    font-size: 1em;
    font-weight: 700;
    background: linear-gradient(135deg, #00e676, #00c853);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    margin-bottom: 15px;
}

.voltage-value {
    font-size: 2em;
    font-weight: 700;
    color: #212121;
    font-family: 'Courier New', monospace;
    margin: 10px 0;
}

.voltage-unit {
    font-size: 0.9em;
    color: #757575;
    font-weight: 600;
}

@media (max-width: 1024px) {
    .stats-grid {
        grid-template-columns: repeat(2, 1fr);
        grid-template-rows: auto;
    }
    .stats-grid .stat-card:nth-child(1) { grid-column: 1; grid-row: 1; }
    .stats-grid .stat-card:nth-child(2) { grid-column: 2; grid-row: 1 / 3; }
    .stats-grid .stat-card:nth-child(3) { grid-column: 2; grid-row: 3; }
    .stats-grid .stat-card:nth-child(4) { grid-column: 1; grid-row: 2; }
    .stats-grid .stat-card:nth-child(5) { grid-column: 1; grid-row: 3; }
    .stats-grid .stat-card:nth-child(6) { grid-column: 1; grid-row: 4; }
}

@media (max-width: 768px) {
    body { padding: 15px; }
    .dashboard { padding: 20px; }
    h1 { font-size: 1.8em; }
    h2 { font-size: 1.2em; }
    .header { 
        flex-direction: column; 
        gap: 15px; 
        align-items: flex-start; 
    }
    .header-center {
        width: 100%;
        margin: 0;
        align-items: flex-start;
    }
    .instructor-info { font-size: 0.9em; }
    .student-group {
        flex-direction: column;
        align-items: flex-start;
        gap: 5px;
    }
    .student-list {
        flex-direction: column;
        gap: 5px;
    }
    .student-info { font-size: 0.85em; }
    .header-right {
        width: 100%;
        justify-content: space-between;
    }
    .stats-grid { 
        grid-template-columns: 1fr;
        gap: 12px; 
    }
    .stats-grid .stat-card:nth-child(1),
    .stats-grid .stat-card:nth-child(2),
    .stats-grid .stat-card:nth-child(3),
    .stats-grid .stat-card:nth-child(4),
    .stats-grid .stat-card:nth-child(5),
    .stats-grid .stat-card:nth-child(6) {
        grid-column: 1;
        grid-row: auto;
    }
    .battery-grid { 
        grid-template-columns: repeat(3, 1fr); 
        gap: 12px; 
    }
    .protection-grid { grid-template-columns: 1fr; }
}

@media (max-width: 480px) {
    body { padding: 10px; }
    .dashboard { padding: 15px; border-radius: 16px; }
    h1 { font-size: 1.5em; }
    h2 { font-size: 1.1em; }
    .stat-card { padding: 15px; }
    .stat-icon { font-size: 2em; }
    .stat-value { font-size: 1.4em; }
    .battery-grid { 
        grid-template-columns: repeat(2, 1fr);
        gap: 10px; 
    }
    .battery-cell { padding: 15px 10px; }
    .voltage-value { font-size: 1.6em; }
    .circle { width: 150px; height: 150px; }
    .inner-circle { width: 110px; height: 110px; }
    .soc-percentage { font-size: 2em; }
    .stat-card.soc-card { min-height: 240px; padding: 20px; }
}
)rawliteral");
}

#endif // BMS_HTML_STYLES_H