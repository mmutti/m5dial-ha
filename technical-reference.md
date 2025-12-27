# M5StickC Plus Tricorder - Technical Reference

## Architecture Overview

The firmware follows a modular app-based architecture with:
- **App System**: Enum-based app identification with state machine
- **Sensor Abstraction**: Auto-detection at boot with enable/disable flags
- **Display Management**: Double-buffered sprite rendering
- **Input Handling**: Unified button and encoder input with long-press detection

---

## File Structure

```
m5stick-c-tricorder/
├── m5stick-c-tricorder.ino   # Main firmware source
├── README.md                  # Project overview
├── manual.md                  # User manual
└── technical-reference.md     # This file
```

---

## Dependencies

| Library | Version | Purpose |
|---------|---------|----------|
| M5StickCPlus | 0.1.1+ | Hardware abstraction |
| SensirionI2cScd4x | 1.1.0+ | CO2 sensor driver |
| BME68x Sensor library | 1.3+ | BME688 sensor driver (ENV Pro) |
| BSEC2 Software Library | 1.10+ | Bosch IAQ algorithm (ENV Pro) |
| Adafruit MLX90614 | 2.1.5+ | NCIR temperature sensor |
| SparkFun MAX3010x | Latest | Heart rate sensor |
| VL53L0X (pololu) | Latest | TOF distance sensor |
| UNIT_MINISCALE | 0.0.1+ | MiniScale weight sensor |
| SunSet | Latest | Solar position calculations |
| Wire | Built-in | I2C communication |
| Preferences | Built-in | Flash storage |
| WiFi | Built-in | NTP time sync |

---

## Constants and Definitions

### GPIO Pins

```cpp
#define BUZZER_PIN 2
#define LED_PIN 10
#define PIR_PIN 33
#define IR_SEND_PIN 9
#define IR_RECEIVE_PIN 36
// Encoder I2C: GPIO 0 (SDA), 26 (SCL)
// Grove Port: GPIO 32 (SDA), 33 (SCL)
```

### I2C Addresses

```cpp
#define MINIENCODERC_ADDR 0x42
#define SCD4X_I2C_ADDR 0x62
#define BME68X_I2C_ADDR_HIGH 0x77  // ENV Pro (SDO high)
#define BME68X_I2C_ADDR_LOW 0x76   // ENV Pro (SDO low)
#define MLX90614_I2C_ADDR 0x5A
#define MAX30102_I2C_ADDR 0x57
#define VL53L0X_I2C_ADDR 0x29
#define MINISCALE_I2C_ADDR 0x26
```

### Timing Constants

```cpp
#define LONG_PRESS_MS 500
#define SAMPLE_INTERVAL_MS 600000      // 10 min (CO2 history)
#define HR_SAMPLE_INTERVAL_MS 300000   // 5 min (HR history)
#define SAVE_INTERVAL_MS 600000        // Flash save interval
```

### History Buffers

```cpp
#define HISTORY_SIZE 144       // 24h @ 10min intervals
#define HR_HISTORY_SIZE 144    // 12h @ 5min intervals
```

---

## Data Structures

### App System

```cpp
enum AppState { APP_RUNNING, APP_MENU };
enum AppID { 
    APP_AIR_QUALITY, 
    APP_STOPWATCH, 
    APP_TIMER, 
    APP_MOTION, 
    APP_NCIR, 
    APP_HEARTRATE, 
    APP_IR_REMOTE,
    APP_LEVEL,
    APP_DISTANCE,
    APP_TOF_COUNTER,
    APP_SCALE,
    APP_WEB_FILES,
    APP_SUN_MOON,
    APP_HOME_ASSISTANT,
    APP_SETTINGS,
    APP_POWER_OFF,
    APP_COUNT 
};

const char* appNames[];        // Display names
bool appEnabled[APP_COUNT];    // Detection flags
```

### Timer States

```cpp
enum TimerState { 
    TIMER_SETTING,    // Adjusting time
    TIMER_RUNNING,    // Counting down
    TIMER_PAUSED,     // Paused
    TIMER_FINISHED    // Alarm playing
};
```

### History Storage

```cpp
// CO2/Temperature history (24h)
uint16_t co2_history[HISTORY_SIZE];
int16_t temp_history[HISTORY_SIZE];    // temp * 10
uint8_t hum_history[HISTORY_SIZE];

// Heart rate history (12h)
uint8_t hr_history[HR_HISTORY_SIZE];
```

---

## Core Functions

### Initialization

```cpp
void setup()
```
1. Initialize M5StickC Plus hardware
2. Initialize buzzer (LEDC PWM)
3. Initialize encoder on Wire bus
4. Initialize PIR pin
5. Scan I2C for sensors and enable apps
6. Set default app based on detected sensors
7. Initialize CO2 sensor
8. Setup display sprite
9. Sync time via WiFi/NTP
10. Load history from flash

### Main Loop

```cpp
void loop()
```
1. Update M5 button states
2. Read CO2 sensor (if available)
3. Handle app input
4. Render current app
5. Periodic history sampling
6. Periodic flash saves

### Sensor Detection

```cpp
// In setup(), for each sensor:
Wire.beginTransmission(ADDR);
if (Wire.endTransmission() == 0) {
    // Sensor found, initialize and enable app
    appEnabled[APP_X] = true;
}
```

### Input Handling

```cpp
void handleAppInput()
```
- Reads encoder value and button states
- Detects short press vs long press
- Routes input to current app or menu
- Long press always opens menu

### Rendering

```cpp
void renderCurrentApp()
```
- Clears sprite buffer
- Calls appropriate app function
- Pushes sprite to display

---

## App Implementation Pattern

Each app follows this structure:

```cpp
void app_example() {
    tftSprite.fillRect(0, 0, 135, 240, BLACK);
    drawHeader();
    
    // Title
    tftSprite.setTextColor(CYAN);
    tftSprite.drawString("APP NAME", x, 20);
    
    // Sensor reading (if applicable)
    // ...
    
    // Display data
    // ...
    
    // History graph (if applicable)
    // ...
    
    // Controls hint
    tftSprite.setTextColor(0x7BEF);
    tftSprite.drawString("[HOLD] Menu", 35, 180);
    
    tftSprite.pushSprite(0, 0);
}
```

---

## Buzzer/Melody System

### PWM Configuration

```cpp
void buzzerInit() {
    ledcSetup(0, 5000, 8);      // Channel 0, 5kHz, 8-bit
    ledcAttachPin(BUZZER_PIN, 0);
}

void buzzerTone(uint16_t freq) {
    ledcWriteTone(0, freq);
}

void buzzerMute() {
    ledcWriteTone(0, 0);
}
```

### Melody Format

```cpp
const int16_t alarmMelody[] = {
    NOTE_X, duration,  // Positive duration = normal note
    NOTE_Y, -duration, // Negative duration = dotted note (1.5x)
    // ...
};
const int alarmTempo = 120;  // BPM
```

### Playback

```cpp
void playAlarmTick()
```
- Non-blocking melody playback
- Advances through notes based on timing
- Loops when complete

---

## Heart Rate Algorithm

### Beat Detection

Custom derivative-based peak detection:

```cpp
hrIrDelta = irValue - hrIrPrev;

// Detect peak: rising->falling transition
if (hrIrDeltaPrev > 100 && hrIrDelta < -100 && !hrBeatState) {
    // Beat detected
    long delta = millis() - hrLastBeat;
    if (delta > 300 && delta < 1500) {
        float bpm = 60000.0 / delta;
        // Store and average
    }
}
```

### Sensor Configuration

```cpp
heartRateSensor.softReset();
delay(100);
heartRateSensor.setup(0x1F, 4, 2, 400, 411, 4096);
// ledBrightness=31, sampleAvg=4, ledMode=2(Red+IR),
// sampleRate=400, pulseWidth=411, adcRange=4096
```

---

## TOF Distance Sensor

### Initialization

```cpp
VL53L0X tofSensor;
TwoWire* tofBus = NULL;

// Detection and init
Wire.beginTransmission(VL53L0X_I2C_ADDR);
if (Wire.endTransmission() == 0) {
    tofSensor.setBus(&Wire);
    tofSensor.setTimeout(500);
    if (tofSensor.init()) {
        tofBus = &Wire;
        tofSensor.startContinuous();
        tofInitialized = true;
    }
}
```

### Reading Distance

```cpp
tofDistance = tofSensor.readRangeContinuousMillimeters();
// Returns 8190 when out of range
// Valid range: ~30mm to ~2000mm
```

### TOF Counter Algorithm

Uses hysteresis to count objects passing in front of sensor:

```cpp
// State variables
bool tofObjectPresent = false;
uint16_t tofThresholdNear = 200;  // Object detected threshold
uint16_t tofThresholdFar = 400;   // Object left threshold

// Detection logic
if (!tofObjectPresent && tofDistance < tofThresholdNear) {
    tofObjectPresent = true;  // Object entered
} else if (tofObjectPresent && tofDistance > tofThresholdFar) {
    tofObjectPresent = false;
    tofCount++;  // Count when object leaves
}
```

### Hardware Note

TOF Hat uses the 8-pin header (same as Encoder Hat). They cannot be used simultaneously. When TOF Hat is detected, encoder-based controls are not available - apps use button-only controls.

---

## ENV Pro Sensor (BME688 with BSEC2)

### Overview

The ENV Pro unit contains a BME688 sensor that provides:
- **IAQ (Indoor Air Quality)** index (0-500)
- **Temperature** (°C)
- **Humidity** (%)
- **Pressure** (hPa)
- **Gas Resistance** (kOhm)

### BSEC2 Library

The Bosch BSEC2 library provides AI-based gas sensing for IAQ calculations:

```cpp
#include <bsec2.h>

Bsec2 envProSensor;
bool envProInitialized = false;
uint8_t envProI2CAddr = 0;  // Detected address (0x76 or 0x77)
```

### Initialization

```cpp
// Detection on both I2C buses at both addresses
Wire.beginTransmission(BME68X_I2C_ADDR_HIGH);
if (Wire.endTransmission() == 0) {
    envProBus = &Wire;
    envProI2CAddr = BME68X_I2C_ADDR_HIGH;
}

// BSEC2 initialization with callback
bsecSensor sensorList[] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS
};

if (envProSensor.begin(envProI2CAddr, *envProBus)) {
    envProSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_ULP);
    envProSensor.attachCallback(envProDataCallback);
    envProInitialized = true;
}
```

### Callback Data Handling

```cpp
void envProDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
    for (uint8_t i = 0; i < outputs.nOutputs; i++) {
        const bsecData output = outputs.output[i];
        switch (output.sensor_id) {
            case BSEC_OUTPUT_IAQ:
                envProIAQ = output.signal;
                envProIAQAccuracy = output.accuracy;  // 0-3
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                envProTemperature = output.signal;
                break;
            // ... other outputs
        }
    }
}
```

### IAQ Accuracy Levels

| Accuracy | Meaning |
|----------|----------|
| 0 | Sensor stabilizing |
| 1 | Low accuracy (calibrating) |
| 2 | Medium accuracy |
| 3 | High accuracy (fully calibrated) |

Full calibration requires ~24 hours of continuous operation.

### IAQ Index Interpretation

| IAQ Range | Air Quality |
|-----------|-------------|
| 0-50 | Excellent |
| 51-100 | Good |
| 101-150 | Lightly Polluted |
| 151-200 | Moderately Polluted |
| 201-300 | Heavily Polluted |
| 301-500 | Severely Polluted |

### Air Quality Alarm Mode

The Air Quality app includes an alarm mode that triggers when air quality becomes poor:

```cpp
// Alarm thresholds
#define CO2_ALARM_THRESHOLD 1500    // ppm (ASHRAE recommends <1000 for comfort)
#define IAQ_ALARM_THRESHOLD 150     // IAQ index (moderately polluted)

// Alarm state
bool airQualityAlarmArmed = false;
bool airQualityAlarmTriggered = false;

// Trigger logic (in app_air_quality())
bool airQualityBad = false;
if (scd4xBus != NULL && co2_value > CO2_ALARM_THRESHOLD) {
    airQualityBad = true;
}
if (envProInitialized && envProIAQAccuracy >= 1 && envProIAQ > IAQ_ALARM_THRESHOLD) {
    airQualityBad = true;
}
```

The alarm:
- Triggers if **either** CO2 or IAQ exceeds threshold (when respective sensor is connected)
- Only uses IAQ when accuracy >= 1 (sensor has started calibrating)
- Automatically stops when air quality improves
- The alarm uses `triggerAlarm()` for multi-modal alerts (buzzer, LED)

### Wire1 Bus Switching

When ENV Pro is on the Grove port (Wire1), the bus must be temporarily switched for BSEC2 operations:

```cpp
// In loop(), before envProSensor.run()
if (envProInitialized && envProBus == &Wire1) {
    switchWire1ToGrove();
}
envProSensor.run();
if (envProInitialized && envProBus == &Wire1) {
    switchWire1ToInternal();
}
```

---

## Sun & Moon App

### Dependencies

```cpp
#include <sunset.h>  // SunSet library for solar calculations
```

### Location Configuration

```cpp
#define BRESCIA_LAT 45.5416
#define BRESCIA_LON 10.2118
#define BRESCIA_TZ 1  // UTC+1 (CET)
```

### Solar Calculations

```cpp
SunSet sun;
sun.setPosition(BRESCIA_LAT, BRESCIA_LON, BRESCIA_TZ);
sun.setCurrentDate(year, month, day);

int sunrise = (int)sun.calcSunrise();       // Minutes from midnight
int sunset = (int)sun.calcSunset();
int civilDawn = (int)sun.calcCivilSunrise();
int civilDusk = (int)sun.calcCivilSunset();
```

### Moon Phase Algorithm

```cpp
void getMoonPhase(int year, int month, int day, int* phase, int* illum);
```

Returns:
- **phase**: 0-7 (New Moon, Waxing Crescent, First Quarter, Waxing Gibbous, Full Moon, Waning Gibbous, Last Quarter, Waning Crescent)
- **illum**: 0-100 (illumination percentage)

### Helper Functions

```cpp
void formatTime(int minutes, char* buf);           // Convert minutes to HH:MM
void formatSecondsDiff(int seconds, char* buf);    // Format as +/-Xm Xs
const char* getMoonPhaseName(int phase);           // Get phase name string
void drawSunIcon(int x, int y);                    // Draw sun graphic
void drawMoonIcon(int x, int y, int phase);        // Draw moon graphic with phase
```

---

## Home Assistant Integration

### Configuration

```cpp
#define HA_WIFI_SSID "YOUR_WIFI_SSID"
#define HA_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define HA_IP "192.168.1.100"
#define HA_PORT 8123
#define HA_API_TOKEN "YOUR_LONG_LIVED_ACCESS_TOKEN"
```

### State Machine

```cpp
enum HAState { 
    HA_DISCONNECTED,  // WiFi not connected
    HA_CONNECTING,    // WiFi connection in progress
    HA_CONNECTED,     // WiFi connected, ready to send
    HA_SENDING,       // HTTP request in progress
    HA_SUCCESS,       // Command sent successfully
    HA_ERROR          // Error occurred
};
```

### WiFi Connection

```cpp
void haConnectWiFi();     // Start WiFi connection (STA mode)
void haDisconnectWiFi();  // Disconnect and turn off WiFi
```

### API Request

```cpp
bool haSendAlarmCommand();
```

Sends HTTP POST to Home Assistant REST API:
- **Endpoint**: `/api/services/alarm_control_panel/alarm_trigger`
- **Payload**: `{"entity_id": "alarm_control_panel.alarmo"}`
- **Auth**: Bearer token in Authorization header

### Timeouts

| Parameter | Value |
|-----------|-------|
| WiFi connect timeout | 15 seconds |
| HTTP request timeout | 5 seconds |
| Status display time | 2 seconds |

---

## MiniScale Weight Sensor

### Initialization

```cpp
UNIT_SCALES scales;
TwoWire* scalesBus = NULL;

// Detection and init
Wire.beginTransmission(MINISCALE_I2C_ADDR);
if (Wire.endTransmission() == 0) {
    if (scales.begin(&Wire, SDA_PIN, SCL_PIN, MINISCALE_I2C_ADDR)) {
        scalesBus = &Wire;
        scalesInitialized = true;
        scales.setLEDColor(0x001000);  // Green LED
    }
}
```

### Reading Weight

```cpp
float weight = scales.getWeight();      // Weight in grams
int32_t rawADC = scales.getRawADC();    // Raw ADC value
float gap = scales.getGapValue();       // Calibration factor
```

### Tare and Calibration

```cpp
// Tare (zero the scale)
scales.setOffset();

// Adjust calibration factor
scales.setGapValue(newGapValue);
```

### LED Control

```cpp
scales.setLEDColor(0x001000);  // Green
scales.setLEDColor(0x100000);  // Red
scales.setLEDColor(0x000010);  // Blue
```

---

## Multi-Modal Alert System

### Components

The alert system triggers two simultaneous feedback methods:

```cpp
void triggerAlarm() {
    playAlarmTick();      // Buzzer melody
    // LED flash (toggled every 200ms)
}
```

### Internal LED

```cpp
#define LED_PIN 10  // Active LOW

void ledInit() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // OFF
}

void ledOn()  { digitalWrite(LED_PIN, LOW); }
void ledOff() { digitalWrite(LED_PIN, HIGH); }
```

### Alert Timing

| Parameter | Value |
|-----------|-------|
| LED flash interval | 200ms |

---

## Flash Storage

### Preferences API

```cpp
Preferences prefs;
prefs.begin("tricorder", false);

// Save
prefs.putBytes("co2_hist", co2_history, sizeof(co2_history));

// Load
prefs.getBytes("co2_hist", co2_history, sizeof(co2_history));

prefs.end();
```

### Stored Data

- CO2 history array
- Temperature history array
- Humidity history array
- History index and count
- Min/max values with timestamps

---

## Display Layout

### Screen Dimensions

- Width: 135 pixels
- Height: 240 pixels
- Orientation: Portrait (rotation 1)

### Standard Layout

```
┌─────────────────────┐
│ Header (0-14)       │  Date, time, battery
├─────────────────────┤
│ Title (20)          │  App name
│                     │
│ Main Content        │  Sensor readings
│ (40-90)             │
│                     │
│ Graph Area          │  History visualization
│ (95-155)            │
│                     │
│ Controls Hint       │  Button instructions
│ (180)               │
└─────────────────────┘
```

### Color Palette

```cpp
BLACK   0x0000
WHITE   0xFFFF
RED     0xF800
GREEN   0x07E0
BLUE    0x001F
CYAN    0x07FF
YELLOW  0xFFE0
ORANGE  0xFD20
0x7BEF  // Gray-blue (hints, labels)
0x4208  // Dark gray (disabled)
```

---

## Memory Usage

| Component | RAM (bytes) |
|-----------|-------------|
| CO2 History | 288 |
| Temp History | 288 |
| Humidity History | 144 |
| HR History | 144 |
| Display Sprite | ~64KB |
| **Total Global** | ~48KB |

Flash usage: ~1MB (33% of 3MB)

---

## Adding New Apps

1. Add to `AppID` enum
2. Add name to `appNames[]`
3. Add `false` to `appEnabled[]`
4. Add I2C address constant (if sensor)
5. Add sensor object and state variables
6. Implement `app_newapp()` function
7. Add case to `renderCurrentApp()`
8. Add input handling in `handleAppInput()`
9. Add detection logic in `setup()`

---

## Known Limitations

- Heart rate history not persisted to flash
- IR receiver requires external module on GPIO 36
- PIR sensor uses GPIO 33 which conflicts with Grove I2C SCL (auto-disabled when Grove I2C is used)
- TOF Hat and Encoder Hat cannot be used simultaneously (same port)
- No deep sleep implementation
- WiFi only used for initial NTP sync

---

## Serial Debug Output

Baud rate: 115200

Key messages:
- `"M5StickCPlus initializing...OK"`
- `"Detecting connected sensors..."`
- `"  CO2 sensor (SCD4x) detected on Wire1 (Grove) at 0x62"`
- `"  ENV Pro (BME688) detected on Wire1 (Grove) at 0x77"`
- `"ENV Pro (BSEC2) initialized successfully"`
- `"BSEC library version X.X.X.X"`
- `"  Heart Rate sensor (MAX30102) detected at 0x57"`
- `"  TOF sensor (VL53L0X) detected on Wire (Qwiic) at 0x29"`
- `"Starting with app: [name]"`
- `"HR sensor initialized"`
