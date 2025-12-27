# M5Dial Home Assistant Controller - Technical Reference

## Architecture Overview

The firmware follows a screen-based architecture with:
- **Screen System**: Enum-based screen identification with state machine
- **MQTT Integration**: PubSubClient for Home Assistant communication
- **Display Management**: Double-buffered sprite rendering (M5Canvas)
- **Input Handling**: Unified touch, encoder, and button input with long-press detection

---

## File Structure

```
m5dial-ha/
├── m5dial-ha.ino             # Main firmware source
├── credentials.h             # WiFi and MQTT credentials (gitignored)
├── credentials.h.example     # Template for credentials
├── README.md                 # Project overview
├── manual.md                 # User manual
└── technical-reference.md    # This file
```

---

## Dependencies

| Library | Version | Purpose |
|---------|---------|----------|
| M5Dial | Latest | Hardware abstraction (display, encoder, touch, RTC) |
| PubSubClient | 2.8+ | MQTT client |
| WiFi | Built-in | WiFi connectivity |
| Preferences | Built-in | Flash storage |

---

## Constants and Definitions

### Display Constants

```cpp
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define CENTER_X 120
#define CENTER_Y 120
```

### Colors

```cpp
#define COLOR_BG        0x0000  // Black
#define COLOR_TEXT      0xFFFF  // White
#define COLOR_ACCENT    0x07FF  // Cyan
#define COLOR_OK        0x03E0  // Dark green
#define COLOR_WARN      0xFFE0  // Yellow
#define COLOR_ERROR     0xF800  // Red
#define COLOR_DIM       0x7BEF  // Gray
#define COLOR_ARMED     0xF800  // Red
#define COLOR_DISARMED  0x03E0  // Dark green
#define COLOR_ORANGE    0xC260  // Dark orange (menu highlight)
```

### Keypad Layout

```cpp
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
#define KEY_WIDTH 50
#define KEY_HEIGHT 38
#define KEYPAD_START_X 45
#define KEYPAD_START_Y 70
#define SIDE_BTN_WIDTH 15
#define SIDE_BTN_HEIGHT (KEY_HEIGHT * 3 + 6)
```

### Timing Constants

```cpp
#define LONG_PRESS_MS 1000
#define WIFI_TIMEOUT_MS 15000
#define MQTT_RECONNECT_INTERVAL 5000
```

### MQTT Topics

```cpp
#define HA_MQTT_COMMAND_TOPIC "home/alarm/set"
#define HA_MQTT_STATE_TOPIC "home/alarm"
#define HA_MQTT_STATUS_TOPIC "home/alarm/status"
```

---

## Data Structures

### Screen System

```cpp
enum AppScreen {
    SCREEN_MAIN,      // Main status screen
    SCREEN_KEYPAD,    // Keypad for code entry
    SCREEN_SENSORS,   // Sensor status list
    SCREEN_SETTINGS   // RTC settings
};

#define MENU_COUNT 3
const char* menuNames[MENU_COUNT] = {"KEYPAD", "SENSORS", "SETTINGS"};
const AppScreen menuScreens[MENU_COUNT] = {SCREEN_KEYPAD, SCREEN_SENSORS, SCREEN_SETTINGS};
```

### Settings Fields

```cpp
enum SettingsField { 
    SET_HOUR, 
    SET_MIN, 
    SET_DAY, 
    SET_MONTH, 
    SET_YEAR, 
    SET_FIELD_COUNT 
};
```

### Connection States

```cpp
enum ConnState { 
    CONN_DISCONNECTED, 
    CONN_CONNECTING, 
    CONN_CONNECTED, 
    CONN_ERROR 
};
```

### Sensor Configuration

```cpp
#define HA_SENSOR_COUNT 11
const char* haSensorTopics[HA_SENSOR_COUNT] = {
    "home-assistant/porta_ingresso/contact",
    "home-assistant/portafinestra_cucina/contact",
    // ... more sensors
};
const char* haSensorNames[HA_SENSOR_COUNT] = {
    "Porta ingresso",
    "PF cucina",
    // ... more names
};
bool haSensorOpen[HA_SENSOR_COUNT] = {false};
```

---

## Core Functions

### Initialization

```cpp
void setup()
```
1. Initialize serial (115200 baud)
2. Initialize M5Dial with encoder enabled, RFID disabled
3. Configure display rotation and create sprite
4. Show startup message
5. Initialize Preferences for RTC sync
6. Set RTC from compile time if not already set
7. Initialize encoder tracking
8. Start WiFi connection
9. Configure MQTT client

### Main Loop

```cpp
void loop()
```
1. Update M5Dial state
2. Handle WiFi connection state machine
3. Handle MQTT connection and reconnection
4. Check for NTP sync completion
5. Handle encoder input
6. Handle touch input
7. Handle button input
8. Draw current screen
9. Push sprite to display
10. 50ms delay for stability

### WiFi Connection

```cpp
void connectWiFi()
```
- Sets WiFi mode to STA (station)
- Begins connection with credentials
- Sets state to CONN_CONNECTING
- Records start time for timeout

### MQTT Connection

```cpp
void connectMqtt()
```
- Connects with client ID "m5dial-ha"
- Subscribes to alarm state topic
- Subscribes to all sensor topics
- Handles connection errors with descriptive messages

### MQTT Callback

```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length)
```
- Parses incoming messages
- Updates `haAlarmState` for alarm topic
- Updates `haSensorOpen[]` array for sensor topics
- Recognizes ON/on/1/true as "open"

---

## Input Handling

### Button Handler

```cpp
void handleButton()
```
- Tracks press duration for long-press detection
- Short press: screen-specific action
- Long press: toggle between Settings and Main

### Encoder Handler

```cpp
void handleEncoder()
```
- Reads encoder delta from last value
- Accumulates steps (requires 2 for action)
- Plays click sound on action
- Routes to screen-specific handler

### Touch Handler

```cpp
void handleTouch()
```
- Checks for `wasPressed()` event
- On keypad: detects CLR, OK, and number key touches
- On main: arms directly or goes to keypad for disarm
- On sensors: returns to main

### Keypad Selection

```cpp
void handleKeypadSelect()
```
- 'C': Clears entered code
- '>': Submits code
- '0'-'9': Appends digit to code

---

## Screen Rendering

### Main Screen

```cpp
void drawMainScreen()
```
- Draws circular menu labels using `drawTextOnArc()`
- Draws status bar (WiFi icon, time, MQTT label)
- Draws alarm state circle with color coding
- Shows sensor summary (open count)

### Keypad Screen

```cpp
void drawKeypadScreen()
```
- Draws status bar
- Shows "ENTER CODE" or masked code
- Draws CLR and OK buttons on sides
- Draws 3x3+1 number grid with selection highlight

### Sensors Screen

```cpp
void drawSensorsScreen()
```
- Draws status bar
- Shows "SENSORS" title
- Two-column layout for sensor list
- Color-coded status dots (green/red)

### Settings Screen

```cpp
void drawSettingsScreen()
```
- Draws status bar
- Shows "SETTINGS" title
- Lists editable fields with highlight
- Shows edit mode indicator (green vs orange)

---

## Helper Functions

### Status Bar

```cpp
void drawStatusBar()
```
- Draws WiFi icon at left (white/gray based on connection)
- Draws time at center
- Draws MQTT label at right (white/red based on connection)

### WiFi Icon

```cpp
void drawWifiIcon(int x, int y, bool connected)
```
- Draws SVG-style wireless signal arcs
- Small dot at bottom (transmitter)
- Three concentric arcs above

### Arc Text

```cpp
void drawTextOnArc(const char* text, float centerAngleDeg, int radius, 
                   uint16_t color, bool highlight, bool clockwise)
```
- Draws text along circular arc
- Supports clockwise and anticlockwise directions
- Draws orange highlight arc behind selected text

---

## RTC Management

### Compile Time Initialization

```cpp
void setRtcFromCompileTime()
```
- Parses `__DATE__` and `__TIME__` macros
- Only sets RTC if year < 2024 (indicates unset)
- Logs action to serial

### NTP Synchronization

```cpp
void syncRtcFromNtp()
```
- Checks if NTP time is valid (after 2020)
- Adds UTC+1 offset for CET timezone
- Updates RTC with NTP time
- Sets `ntpSynced` flag

### Settings Adjustment

```cpp
void adjustSettingsField(int direction)
```
- Gets current RTC datetime
- Adjusts selected field with wraparound
- Constrains year to 2020-2099
- Updates RTC immediately

---

## MQTT Commands

### Alarm Toggle

```cpp
void toggleAlarm()
```
- Checks current alarm state
- Publishes ARM_AWAY or DISARM to command topic

### Code Submission

```cpp
void submitCode()
```
- Determines command based on current state
- Publishes command to MQTT
- Clears code and returns to main screen

---

## Display Layout

### Screen Dimensions

- Width: 240 pixels
- Height: 240 pixels
- Shape: Round (GC9A01 display)
- Orientation: 0 (default)

### Main Screen Layout

```
        ┌─────────────┐
       /   KEYPAD     \
      /                \
     │  WiFi  12:34 MQTT│
     │                  │
     │    ┌────────┐    │
SETTINGS │  ARMED  │ SENSORS
     │    └────────┘    │
     │   All closed     │
      \                /
       \              /
        └─────────────┘
```

### Keypad Layout

```
┌─────────────────────────┐
│   WiFi  12:34  MQTT     │
│                         │
│      ENTER CODE         │
│        ****             │
│                         │
│  C  │ 1 │ 2 │ 3 │  O    │
│  L  │ 4 │ 5 │ 6 │  K    │
│  R  │ 7 │ 8 │ 9 │       │
│     │   │ 0 │   │       │
└─────────────────────────┘
```

---

## Memory Usage

| Component | RAM (bytes) |
|-----------|-------------|
| Display Sprite | ~115KB (240x240x2) |
| MQTT Buffer | 512 |
| Sensor Arrays | ~200 |
| Code Buffer | 9 |
| **Total Estimate** | ~120KB |

Flash usage: Minimal (single .ino file)

---

## Customization

### Adding Sensors

1. Increase `HA_SENSOR_COUNT`
2. Add topic to `haSensorTopics[]`
3. Add display name to `haSensorNames[]`
4. Array `haSensorOpen[]` auto-expands

### Changing Timezone

In `syncRtcFromNtp()`:
```cpp
now += 3600;  // Change offset in seconds
```

### Changing MQTT Topics

Edit defines at top of file:
```cpp
#define HA_MQTT_COMMAND_TOPIC "your/topic/set"
#define HA_MQTT_STATE_TOPIC "your/topic/state"
```

### Adding Menu Items

1. Increase `MENU_COUNT`
2. Add name to `menuNames[]`
3. Add screen to `menuScreens[]`
4. Add new `AppScreen` enum value
5. Implement `drawNewScreen()` function
6. Add case to main loop switch

---

## Known Limitations

- Timezone is hardcoded to UTC+1 (CET)
- Sensor list is hardcoded (requires recompile to change)
- No deep sleep implementation
- No persistent storage of alarm state
- Code is sent but not validated locally

---

## Serial Debug Output

Baud rate: 115200

Key messages:
- `"M5Dial Home Assistant Controller"` - Startup
- `"Connecting to WiFi: <SSID>"` - WiFi attempt
- `"WiFi connected!"` - WiFi success
- `"IP: <address>"` - IP assignment
- `"Connecting to MQTT: <server>:<port>"` - MQTT attempt
- `"MQTT connected!"` - MQTT success
- `"MQTT failed, rc=<code>"` - MQTT error
- `"MQTT [<topic>]: <message>"` - Incoming message
- `"Sent: <command>"` - Outgoing command
- `"RTC set from compile time: <datetime>"` - RTC init
- `"RTC synced from NTP: <datetime>"` - NTP sync
