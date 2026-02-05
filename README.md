# ESP32 Battery Management System (BMS)

> **Hệ thống quản lý pin LiFePO4 4S với ESP32**  
> GVHD: PGS TS Nguyễn Chiến Trinh  
> SV: Hoàng Văn Quang (B21DCVT363), Phạm Thành Long (B21DCVT275)

---

## 📋 Tổng quan

Hệ thống BMS hoàn chỉnh cho pin LiFePO4 4S (14.4V nominal) với các tính năng:

- ⚡ **Đo lường chính xác**: Điện áp từng cell, dòng điện, nhiệt độ
- 🛡️ **Bảo vệ toàn diện**: Over/Under Voltage, Over Current, Over/Under Temperature
- ⚖️ **Cân bằng cell**: Cân bằng thụ động với chu kỳ ON/OFF
- 🔋 **Ước lượng SOC**: Thuật toán Coulomb Counting + OCV với hiệu chỉnh tự động
- ❤️ **Ước lượng SOH**: Mô hình linear aging dựa trên số chu kỳ
- 📺 **Màn hình DWIN**: Hiển thị thời gian thực qua UART
- 🌐 **Web Dashboard**: Giao diện web responsive với WiFi AP

---

## 🔧 Phần cứng

### Thông số pin dựa datasheet LiFePO4 EVH-32700
- **Loại cell**: LiFePO4 EVH-32700
- **Cấu hình**: 4S (4 cell nối tiếp)
- **Dung lượng**: 6000mAh (6Ah)
- **Điện áp**: 9.0V - 14.6V
- **Chu kỳ sạc**: 2000 cycles @ 80% SOH

### ESP32 Pinout

#### Đo điện áp (ADC)
```
PIN_T1 = 34  // Tap point 1 (Cell 1-4)
PIN_T2 = 35  // Tap point 2 (Cell 2-4)
PIN_T3 = 32  // Tap point 3 (Cell 3-4)
PIN_T4 = 33  // Tap point 4 (Cell 4)
```

#### Đo dòng & nhiệt độ
```
PIN_I    = 36  // ACS712-20A Current Sensor
PIN_TEMP = 39  // LM35 Temperature Sensor
```

#### Bảo vệ (MOSFETs)
```
PIN_CHG = 22  // Charge MOSFET Control
PIN_DSG = 23  // Discharge MOSFET Control
```

#### Cân bằng
```
PIN_BAL1 = 25  // Balancing Cell 1
PIN_BAL2 = 26  // Balancing Cell 2
PIN_BAL3 = 27  // Balancing Cell 3
PIN_BAL4 = 14  // Balancing Cell 4
```

#### DWIN Display (UART)
```
Serial2: TX=17, RX=16
Baud: 115200
```

---

## 📁 Cấu trúc dự án

```
ESP32_BMS/
├── src/
│   ├── main.cpp                  # Main program (tích hợp cả 2)
│   │
│   ├── [QUANG] Hardware & Sensors
│   ├── BMSSensors.cpp            # Đo điện áp, dòng, nhiệt độ
│   ├── BMSProtection.cpp         # Bảo vệ OV/UV/OC/OT
│   ├── BMSBalancing.cpp          # Cân bằng cell thụ động
│   ├── BMSDwin.cpp               # Driver màn hình DWIN
│   │
│   ├── [LONG] Algorithms & Monitoring
│   ├── SOCEstimator.cpp          # Thuật toán SOC
│   ├── SOHEstimator.cpp          # Thuật toán SOH
│   ├── BMSData.cpp               # Cấu trúc dữ liệu & JSON
│   └── BMSHTML.cpp               # Web Dashboard & WiFi AP
│
├── include/
│   ├── bmsensors.h
│   ├── bmsprotection.h
│   ├── bmsalancing.h
│   ├── bmsdwin.h
│   ├── bms_html.h
│   ├── bms_html_styles.h [LONG]
│   ├── bms_html_scripts.h [LONG]
│   ├── SOCEstimator.h [LONG]
│   ├── SOHEstimator.h
│   ├── BMSData.h
│   └── BMSHTML.h
│
├── platformio.ini
└── README.md
```

---

## 🚀 Cài đặt

### Yêu cầu
- **PlatformIO** hoặc **Arduino IDE**
- **ESP32 Dev Board**
- **Thư viện**:
  - ArduinoJson
  - Preferences (built-in)
  - WiFi (built-in)
  - WebServer (built-in)


#### Arduino IDE
1. Mở `src/main.cpp`
2. Chọn board: **ESP32 Dev Module**
3. Cấu hình:
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Partition: Default 4MB
4. Compile & Upload

---

## 🌐 Web Dashboard

### Truy cập
1. Kết nối WiFi:
   - **SSID**: `ESP32_BMS`
   - **Password**: `12345678`

2. Mở trình duyệt:
   - URL: `http://192.168.4.1`

### Tính năng Dashboard
- 📊 Hiển thị real-time (cập nhật 1s)
- 🔋 SOC với circular progress bar
- ⚡ Điện áp, dòng, nhiệt độ
- 🛡️ Trạng thái bảo vệ
- ⚖️ Trạng thái cân bằng
- 🚨 Cảnh báo/Alarm

---

## 💻 Serial Commands

Kết nối Serial Monitor (115200 baud):

### Monitoring
```
soc         - Hiển thị SOC debug
soh         - Hiển thị SOH debug
sensors     - Hiển thị sensor readings
protection  - Hiển thị protection status
balance     - Hiển thị balancing status
dwin        - Hiển thị DWIN status
data        - Hiển thị BMS data struct
json        - In JSON API output
```

### Calibration
```
reset_soh       - Reset SOH về 100%
reset_cycles    - Reset cycle counter
cal_soh 5.5     - Hiệu chỉnh SOH (ví dụ: 5.5Ah)
```

### System
```
wifi            - Hiển thị WiFi info
clients         - Số client đang kết nối
help            - Hiển thị menu lệnh
```

---

## 🔬 Thuật toán

### SOC Estimation
**Hybrid Method**:
1. **OCV Lookup**: Khởi tạo SOC từ điện áp pack tham khảo link https://www.ecoflow.com/us/blog/lifepo4-voltage-chart
2. **Coulomb Counting**: Tích hợp dòng điện theo thời gian
3. **Temperature Compensation**: Bù nhiệt độ cho dung lượng dựa trên datasheet LiFePO4 EVH-32700
4. **Auto Recalibration**:
   - Full charge: V ≥ 14.6V, idle ≥ 30min
   - OCV sync: idle ≥ 2 hours

### SOH Estimation
**Linear Aging Model**:
```
SOH = 100% - (total_cycles × 0.01%)
```
- Mỗi chu kỳ sạc/xả: -0.01% SOH
- 2000 cycles → 80% SOH (End of Life) dựa trên datasheet LiFePO4 EVH-32700
- Lưu dữ liệu vào NVS flash mỗi 5 phút

### Cell Balancing
**Passive Balancing**:
- Bật khi: Delta > 100mV, V_max ≥ 3.5V, Idle
- Tắt khi: Delta < 30mV hoặc có dòng
- Chu kỳ: 5s ON / 5s OFF (tránh quá nhiệt)

### Protection dựa trên datasheet
**Hysteresis với Recovery Timer**:
- **Charge**: OV (3.65V), OC (1.4A), OT (45°C)
- **Discharge**: UV (2.5V), OC (-6A), OT (60°C)
- Recovery: 5 giây sau khi điều kiện trở về bình thường

---

## 📊 JSON API

### Endpoint: `/bms`

**Response Example**:
```json
{
  "measurement": {
    "cellVoltages": [
      {"cell": 1, "voltage": "3.320"},
      {"cell": 2, "voltage": "3.315"},
      {"cell": 3, "voltage": "3.318"},
      {"cell": 4, "voltage": "3.322"}
    ],
    "packVoltage": "13.28",
    "avgCellVoltage": "3.320",
    "current": "0.00",
    "packTemperature": "25.5"
  },
  "calculation": {
    "soc": "85.0",
    "soh": "98.5",
    "remainingCapacity": "5.910",
    "totalCycles": "15.0",
    "remainingCycles": "1985"
  },
  "status": {
    "charging": "idle",
    "balancing": {
      "active": false,
      "cells": []
    }
  },
  "protection": {
    "overVoltage": "normal",
    "underVoltage": "normal",
    "overCurrentCharge": "normal",
    "overCurrentDischarge": "normal",
    "overTempCharge": "normal",
    "overTempDischarge": "normal"
  },
  "alerts": []
}
```

---

## 🛡️ Ngưỡng bảo vệ dựa trên datasheet LiFePO4 EVH-32700

### Charging Protection
| Parameter | Warning | Trip | Release | Recovery Time |
|-----------|---------|------|---------|---------------|
| Cell OV   | 3.45V   | 3.65V| 3.40V   | 5s            |
| Current   | 1.0A    | 1.4A | 0.8A    | 5s            |
| Temp High | 40°C    | 45°C | 38°C    | 5s            |
| Temp Low  | 5°C     | 0°C  | 3°C     | 5s            |

### Discharging Protection
| Parameter | Warning | Trip | Release | Recovery Time |
|-----------|---------|------|---------|---------------|
| Cell UV   | 3.00V   | 2.50V| 2.90V   | 5s            |
| Current   | -4.0A   | -6.0A| -3.5A   | 5s            |
| Temp High | 55°C    | 60°C | 50°C    | 5s            |
| Temp Low  | -5°C    | -10°C| -8°C    | 5s            |

---

## 📈 Hiệu suất

- **Update Rate**: 100ms (sensors + protection + balancing)
- **SOC Update**: 1s
- **SOH Update**: 10s
- **Web Refresh**: 1s
- **DWIN Update**: 1s
- **Flash Save**: 5 phút

---



