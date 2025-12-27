# M5Dial Home Assistant Controller

A Home Assistant alarm controller for the M5Dial with rotary encoder navigation, touch screen keypad, and MQTT integration.

## Features

- **MQTT Alarm Control**: Arm/disarm Home Assistant alarm via MQTT
- **Touch Screen Keypad**: Phone-like numeric keypad for code entry
- **Rotary Encoder Navigation**: Navigate menus and enter codes using the dial
- **Sensor Monitoring**: Real-time status of door/window sensors
- **RTC Time Display**: Shows current time with NTP synchronization
- **Double-Buffered Display**: Flicker-free 240x240 round display

## Hardware Requirements

- **M5Dial** (ESP32-S3 based with rotary encoder and touch screen)
- WiFi network with MQTT broker (e.g., Home Assistant with Mosquitto)

## Screens

| Screen | Description |
|--------|-------------|
| **Main** | Alarm status display with sensor summary and circular menu |
| **Keypad** | Numeric keypad for entering alarm codes |
| **Sensors** | List of all door/window sensors with open/closed status |
| **Settings** | RTC date/time configuration |

## Installation

### Prerequisites

1. Arduino IDE or arduino-cli
2. ESP32 board support package
3. Required libraries:
   - M5Dial
   - PubSubClient (MQTT)
   - WiFi (built-in)
   - Preferences (built-in)

### Configuration

1. Copy `credentials.h.example` to `credentials.h`
2. Fill in your values:
   ```cpp
   #define HA_WIFI_SSID "your_wifi_ssid"
   #define HA_WIFI_PASSWORD "your_wifi_password"
   #define HA_MQTT_SERVER "192.168.1.100"
   #define HA_MQTT_PORT 1883
   #define HA_MQTT_USER "mqtt_username"
   #define HA_MQTT_PASSWORD "mqtt_password"
   ```

### Compile and Upload

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:m5stack_dial

# Upload
arduino-cli upload --fqbn esp32:esp32:m5stack_dial -p /dev/ttyUSB0
```

## Usage

### Controls

| Input | Action |
|-------|--------|
| **Rotary Encoder** | Navigate menus, select keys, adjust settings |
| **Button Press (Short)** | Select/confirm current item |
| **Button Press (Long)** | Go to Settings / Back to Main |
| **Touch Screen** | Tap keys on keypad, tap alarm status to arm/disarm |

### Main Screen

- Displays current alarm state (ARMED/DISARMED/PENDING/TRIGGERED)
- Shows sensor summary (all closed or X open)
- Circular menu around the border: KEYPAD, SENSORS, SETTINGS
- Rotate encoder to highlight menu item, press to select
- Touch the alarm status to arm (when disarmed) or go to keypad (when armed)

### Keypad Screen

- Enter alarm code using touch or encoder
- CLR button on left to clear code
- OK button on right to submit
- Code is masked with asterisks

### Sensors Screen

- Two-column list of all configured sensors
- Green dot = closed, Red dot = open
- Touch or press to return to main screen

### Settings Screen

- Adjust Hour, Minute, Day, Month, Year
- Rotate encoder to navigate fields
- Press to enter edit mode, rotate to adjust value
- Long press to return to main screen

## MQTT Topics

| Topic | Direction | Values |
|-------|-----------|--------|
| `home/alarm/set` | Publish | ARM_AWAY, ARM_HOME, DISARM |
| `home/alarm` | Subscribe | armed_away, armed_home, disarmed, pending, triggered |
| `home-assistant/<sensor>/contact` | Subscribe | ON (open), OFF (closed) |

## Boot Behavior

On startup, the device:
1. Initializes display and encoder
2. Sets RTC from compile time if not already set
3. Connects to WiFi (15 second timeout)
4. Syncs RTC from NTP server
5. Connects to MQTT broker
6. Subscribes to alarm state and sensor topics

## License

MIT License
