#ifndef BMS_HTML_SCRIPTS_H
#define BMS_HTML_SCRIPTS_H

#include <Arduino.h>

/**
 * ═══════════════════════════════════════════════════════════
 *  BMS HTML SCRIPTS - JavaScript
 * ═══════════════════════════════════════════════════════════
 */

String getHTMLScripts() {
    return F(R"rawliteral(
const CONFIG = {
    API_ENDPOINT: '/bms',
    UPDATE_INTERVAL: 2000
};

function getSOCColor(percent) {
    if (percent > 80) return '#00e676';
    if (percent > 60) return '#66bb6a';
    if (percent > 40) return '#ffca28';
    if (percent > 20) return '#ffa726';
    return '#ff5252';
}

function createSimpleBatteryCell(cellNum, voltage, isBalancing = false) {
    const balancingClass = isBalancing ? 'balancing' : '';
    return `
        <div class="battery-cell ${balancingClass}">
            <div class="cell-label">Cell ${cellNum}</div>
            <div class="voltage-value">${voltage.toFixed(3)}<span class="voltage-unit">V</span></div>
        </div>
    `;
}

function updateElement(id, value, suffix = '') {
    const element = document.getElementById(id);
    if (!element) return;
    
    const newValue = value + suffix;
    if (element.textContent !== newValue) {
        element.style.opacity = '0.5';
        setTimeout(() => {
            element.textContent = newValue;
            element.style.opacity = '1';
        }, 150);
    }
}

function updateSOCCircular(soc) {
    const socPercent = parseFloat(soc);
    const percentageEl = document.getElementById('socPercentage');
    const circleEl = document.getElementById('socCircle');
    
    if (!percentageEl || !circleEl) return;
    
    const color = getSOCColor(socPercent);
    const degrees = socPercent * 3.6;
    
    percentageEl.style.opacity = '0.5';
    setTimeout(() => {
        percentageEl.textContent = socPercent.toFixed(0) + '%';
        percentageEl.style.opacity = '1';
    }, 150);
    
    circleEl.style.background = `conic-gradient(${color} ${degrees}deg, #e0e0e0 0deg)`;
}

function updateChargingStatus(status) {
    const badge = document.getElementById('chargingStatus');
    const icon = badge.querySelector('.status-icon');
    const text = badge.querySelector('.status-text');
    
    badge.classList.remove('charging', 'discharging', 'idle');
    
    if (status === 'charging') {
        badge.classList.add('charging');
        icon.textContent = '🔌';
        text.textContent = 'Charging';
    } else if (status === 'discharging') {
        badge.classList.add('discharging');
        icon.textContent = '⚡';
        text.textContent = 'Discharging';
    } else {
        badge.classList.add('idle');
        icon.textContent = '⏸️';
        text.textContent = 'Idle';
    }
}

function updateProtectionStatus(protectionData) {
    const protections = {
        'protectOV': { 
            data: protectionData.overVoltage, 
            label: 'Over Voltage (Sạc)' 
        },
        'protectUV': { 
            data: protectionData.underVoltage, 
            label: 'Under Voltage (Xả)' 
        },
        'protectOCCharge': { 
            data: protectionData.overCurrentCharge, 
            label: 'Over Current (Sạc)' 
        },
        'protectOCDischarge': { 
            data: protectionData.overCurrentDischarge, 
            label: 'Over Current (Xả)' 
        },
        'protectOT': { 
            data: protectionData.overTemperature, 
            label: 'Over Temperature' 
        }
        // XÓA protectSC
    };

    for (const [id, info] of Object.entries(protections)) {
        const element = document.getElementById(id);
        if (!element) continue;
        
        const statusEl = element.querySelector('.protection-status');
        
        element.classList.remove('alert', 'alarm');
        
        if (info.data === 'alarm') {
            element.classList.add('alarm');
            statusEl.textContent = 'ALARM!';
        } else if (info.data === 'warning') {
            element.classList.add('alert');
            statusEl.textContent = 'Warning';
        } else {
            statusEl.textContent = 'Normal';
        }
    }
}

function updateAlerts(alerts) {
    const container = document.getElementById('alertsContainer');
    
    if (!alerts || alerts.length === 0) {
        container.classList.add('hidden');
        return;
    }
    
    container.classList.remove('hidden');
    container.innerHTML = alerts.map(alert => `
        <div class="alert ${alert.severity}">
            <div class="alert-icon">${alert.severity === 'critical' ? '🚨' : '⚠️'}</div>
            <div class="alert-text">${alert.message}</div>
        </div>
    `).join('');
}

async function fetchBMSData() {
    try {
        const response = await fetch(CONFIG.API_ENDPOINT);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const data = await response.json();
        
        if (data.measurement?.packVoltage) {
            const packVolt = parseFloat(data.measurement.packVoltage).toFixed(2);
            updateElement('packVolt', packVolt, ' V');
        }
        
        if (data.calculation?.soc) {
            const soc = parseFloat(data.calculation.soc).toFixed(1);
            updateSOCCircular(soc);
        }
        
        if (data.calculation?.soh) {
            const soh = parseFloat(data.calculation.soh).toFixed(1);
            updateElement('soh', soh, ' %');
        }
        
        if (data.measurement?.current !== undefined) {
            const current = parseFloat(data.measurement.current).toFixed(2);
            updateElement('current', current, ' A');
        }
        
        if (data.measurement?.packTemperature) {
            const temp = parseFloat(data.measurement.packTemperature).toFixed(1);
            updateElement('packTemp', temp, ' °C');
        }
        
        if (data.status?.charging) {
            updateChargingStatus(data.status.charging);
        }
        
        if (data.status?.balancing) {
            const balStatus = data.status.balancing.active ? 'Active' : 'Inactive';
            updateElement('balancingStatus', balStatus);
        }
        
        if (data.protection) {
            updateProtectionStatus(data.protection);
        }
        
        if (data.alerts) {
            updateAlerts(data.alerts);
        }
        
        if (data.measurement?.cellVoltages && data.status?.balancing) {
            const balancingCells = data.status.balancing.cells || [];
            const batteryHTML = data.measurement.cellVoltages
                .map(cell => {
                    const isBalancing = balancingCells.includes(cell.cell);
                    return createSimpleBatteryCell(cell.cell, parseFloat(cell.voltage), isBalancing);
                })
                .join('');
            
            const display = document.getElementById('batteryDisplay');
            if (display.innerHTML !== batteryHTML) {
                display.style.opacity = '0.5';
                setTimeout(() => {
                    display.innerHTML = batteryHTML;
                    display.style.opacity = '1';
                }, 200);
            }
        }
        
        console.log('✅ Data updated:', new Date().toLocaleTimeString());
        
    } catch (error) {
        console.error('❌ Fetch error:', error);
        document.getElementById('batteryDisplay').innerHTML = 
            '<div class="loading">⚠️ Connection Error</div>';
    }
}

(function init() {
    console.log('🚀 BMS Dashboard initialized');
    console.log('⚙️ Update interval:', CONFIG.UPDATE_INTERVAL + 'ms');
    
    ['packVolt', 'soh', 'current', 'packTemp', 'batteryDisplay', 'socPercentage', 'socCircle'].forEach(id => {
        const element = document.getElementById(id);
        if (element) {
            element.style.transition = 'opacity 0.3s ease';
        }
    });
    
    fetchBMSData();
    setInterval(fetchBMSData, CONFIG.UPDATE_INTERVAL);
    
    console.log('✅ Auto-refresh enabled');
})();
)rawliteral");
}

#endif // BMS_HTML_SCRIPTS_H