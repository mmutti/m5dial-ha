# M5StickC Plus Tricorder

A multi-sensor monitoring device firmware for the M5StickC Plus with modular app system and automatic sensor detection.

## Features

- **Modular App System**: Multiple apps accessible via menu with automatic sensor detection
- **Dynamic Sensor Detection**: Apps are enabled/disabled based on connected hardware at boot
- **12/24 Hour History Graphs**: Visual tracking of sensor data over time
- **Min/Max Tracking**: Records extreme values with timestamps
- **Battery Status**: Header displays battery percentage, remaining time, and charging status
- **Multi-Modal Alerts**: Alarms trigger buzzer melody and LED flashing

## Supported Apps

| App | Sensor | I2C Address | Description |
|-----|--------|-------------|-------------|
| Air Quality | SCD4x (CO2), SGP30 (TVOC/eCO2), and/or ENV Pro (BME688) | 0x62 / 0x58 / 0x77 | CO2, eCO2, TVOC, IAQ, temperature, humidity, pressure with 24h graph and weather forecast |
| Stopwatch | None | - | Simple stopwatch with start/stop/reset |
| Timer | None | - | Countdown timer with alarm melody |
| Motion | PIR Sensor | GPIO 33 | Motion detection alarm |
| NCIR Temp | MLX90614 | 0x5A | Non-contact infrared temperature |
| Heart Rate | MAX30102 | 0x57 | Heart rate monitor with 12h graph |
| IR Remote | IR LED + Receiver | GPIO 9/36 | Learn and replay IR remote commands |
| 3D Level | MPU6886 (built-in) | - | Bubble level with angle alarm |
| Distance | VL53L0X (TOF Hat) | 0x29 | Laser distance measurement (mm/cm/in) |
| TOF Counter | VL53L0X (TOF Hat) | 0x29 | Count objects passing in front of sensor |
| Scale | MiniScale Unit | 0x26 | Weight measurement (g/kg/oz/lb) |
| Sun & Moon | None | - | Sunrise/sunset times, moon phase and illumination |
| Home Assistant | WiFi + MQTT | - | Arm/disarm Home Assistant alarm via MQTT, monitor door/window sensors |

## Hardware Requirements

- **M5StickC Plus** (ESP32-based)
- **M5StickC Encoder Hat** (optional, for rotary encoder input)
- One or more supported sensors/hats:
  - SCD4x CO2 sensor (connected to encoder hat I2C port or Grove port)
  - SGP30 TVOC/eCO2 Unit - volatile organic compounds and estimated CO2
  - ENV Pro Unit (BME688) - IAQ, temperature, humidity, pressure, gas resistance
  - PIR Motion sensor (Grove port - GPIO 32/33)
  - NCIR Hat (MLX90614)
  - Heart Rate Hat (MAX30102)
  - TOF Hat (VL53L0X) - laser distance sensor
  - MiniScale Unit - weight sensor

## Pin Configuration

| Function | GPIO |
|----------|------|
| Buzzer | 2 |
| Internal LED | 10 |
| Encoder I2C SDA | 0 |
| Encoder I2C SCL | 26 |
| Grove Port SDA | 32 |
| Grove Port SCL | 33 |
| PIR Sensor | 33 |

## Installation

### Prerequisites

1. Arduino IDE or arduino-cli
2. ESP32 board support package
3. Required libraries:
   - M5StickCPlus
   - Sensirion I2C SCD4x
   - Adafruit MLX90614
   - SparkFun MAX3010x Sensor Library
   - VL53L0X (pololu)
   - UNIT_MINISCALE
   - IRremote
   - BME68x Sensor library (Bosch)
   - BSEC2 Software Library (Bosch) - for ENV Pro IAQ calculations
   - Adafruit SGP30 - for TVOC/eCO2 sensor
   - SunSet - for solar position calculations

### Compile and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:m5stack_stickc_plus

# Upload
arduino-cli upload --fqbn esp32:esp32:m5stack_stickc_plus -p /dev/ttyUSB0
```

## Usage

### Controls

The device supports two input methods: the optional M5StickC Encoder Hat and the built-in buttons. All apps can be fully operated using just the onboard buttons.

#### General Navigation

| Input | Action |
|-------|--------|
| **Front Button (A) - Short** | App-specific action (select, confirm, arm/disarm) |
| **Front Button (A) - Long** | Open app menu / Go back in sub-menus |
| **Side Button (B)** | Scroll to next item in lists |
| **Encoder Rotation** | Navigate menu / Adjust values |
| **Encoder Button - Short** | Select / Confirm |
| **Encoder Button - Long** | Open app menu |

#### App Menu
- **Encoder/B Button**: Scroll through apps (skips disabled apps)
- **A Button/Encoder Press**: Select app
- Disabled apps (no sensor detected) are shown in gray and skipped during navigation

#### IR Remote App

| State | A Button | B Button | Long Press A |
|-------|----------|----------|--------------|
| Main List | Send command / Browse files | Next item | Open menu |
| File Selection | Load selected file | Next file | Back to main list |
| Command Selection | Send IR command | Next command | Back to file list |

#### Web Files App

| State | A Button | B Button | Long Press A |
|-------|----------|----------|--------------|
| Main Screen | Start WiFi AP | View files | Open menu |
| File List | Delete selected file | Next file | Back to main |
| WiFi Active | Stop WiFi | - | Open menu |

**WiFi AP Credentials:**
- SSID: `m5stickc-tricorder`
- Password: `tricorder`
- IP: `192.168.4.1`

The web interface allows uploading/deleting files and includes a "Format LittleFS" button to reset the filesystem.

### Boot Behavior

On startup, the device:
1. Scans I2C buses for connected sensors
2. Enables apps for detected sensors
3. Starts with the first detected sensor app (priority: Distance > Air Quality > NCIR > Heart Rate > Motion, or IR Remote if none)

## Air Quality App

The Air Quality app supports multiple sensors and displays comprehensive environmental data:

### Supported Sensors
- **SCD4x** (0x62): Real CO2 measurement (preferred)
- **SGP30** (0x58): TVOC and estimated CO2 (eCO2)
- **ENV Pro/BME688** (0x76/0x77): IAQ, temperature, humidity, pressure, gas resistance

### Data Display
| Metric | Color | Source | Notes |
|--------|-------|--------|-------|
| CO2/eCO2 | Magenta | SCD4x or SGP30 | SCD4x preferred when both present |
| TVOC | Gray | SGP30 | With quality level label |
| IAQ | Gray | ENV Pro | With accuracy indicator |
| Temperature | Yellow | SCD4x or ENV Pro | With 24h min/max |
| Humidity | Cyan | SCD4x or ENV Pro | With comfort level |
| Pressure | Orange | ENV Pro | In Pascals, with weather trend |
| Gas | Gray | ENV Pro | Resistance in kOhm |

### TVOC Quality Levels
Based on German Federal Environment Agency guidelines:

| Level | TVOC (ppb) | Label | Recommendation |
|-------|------------|-------|----------------|
| 1 | <150 | Good | No action needed |
| 2 | 150-500 | Ventilate | Increase ventilation |
| 3 | 500-1500 | Poor | Find contamination source |
| 4 | 1500-5000 | Bad | Room should not be used |
| 5 | >5000 | Evacuate | Leave immediately |

### Weather Forecast
When ENV Pro is connected, pressure history is used to predict weather:
- **Rising** pressure: Weather improving
- **Stable** pressure: No change expected
- **Falling** pressure: Weather deteriorating

### Humidity Compensation
When both SGP30 and ENV Pro are connected, humidity data from ENV Pro is used to calibrate the SGP30 for improved accuracy.

### Alarm Triggers
The air quality alarm triggers when:
- CO2 > 1500 ppm (SCD4x)
- eCO2 > 1500 ppm (SGP30)
- TVOC ≥ 1500 ppb (SGP30)
- IAQ > 150 (ENV Pro, when accuracy ≥ 1)

## Home Assistant App

Control your Home Assistant alarm system via MQTT.

### Setup
1. Copy `credentials.h.example` to `credentials.h`
2. Fill in your WiFi and MQTT credentials
3. Configure MQTT topics to match your Home Assistant setup

### Features
- **Arm/Disarm**: Button A toggles alarm state
- **Sensor Monitoring**: Shows open/closed status of door/window sensors
- **Safety Check**: Prevents arming when sensors are open
- **WiFi Status**: Shows connection status in header
- **Disconnect**: Button B disconnects WiFi when connected

### MQTT Topics
- Command: `home/alarm/set` (ARM_AWAY, ARM_HOME, DISARM)
- State: `home/alarm` (armed_away, armed_home, disarmed)
- Sensors: `home-assistant/<sensor>/contact` (ON/OFF)

## License

MIT License

## Author

Built with M5Stack hardware and community libraries.
# m5dial-ha
