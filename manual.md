# M5Dial Home Assistant Controller - User Manual

## Getting Started

### First Boot

1. Power on your M5Dial
2. The device will display "Connecting..." while establishing WiFi connection
3. Once connected, it will sync time from NTP and connect to your MQTT broker
4. The main screen will show the current alarm state

### Initial Setup

Before first use, you must configure your credentials:

1. Copy `credentials.h.example` to `credentials.h`
2. Edit the file with your WiFi and MQTT settings:
   - `HA_WIFI_SSID`: Your WiFi network name
   - `HA_WIFI_PASSWORD`: Your WiFi password
   - `HA_MQTT_SERVER`: IP address of your MQTT broker
   - `HA_MQTT_PORT`: MQTT port (usually 1883)
   - `HA_MQTT_USER`: MQTT username
   - `HA_MQTT_PASSWORD`: MQTT password
3. Compile and upload the firmware

---

## Understanding the Display

### Status Bar
The top of every screen shows:
- **WiFi Icon**: White when connected, gray when disconnected
- **Time**: Current time in HH:MM format (from RTC)
- **MQTT Label**: White when connected, red when disconnected

### Color Coding
- **Cyan**: Accent color, titles, highlights
- **Orange**: Selected menu items, edit mode indicators
- **Green**: OK status, disarmed state, closed sensors
- **Red**: Error status, armed state, open sensors
- **Yellow**: Warning, pending states
- **Gray**: Disabled items, hints

---

## Navigation

### Using the Rotary Encoder
- **Rotate clockwise**: Move to next item / Increase value
- **Rotate counter-clockwise**: Move to previous item / Decrease value
- The encoder requires 2 steps to register an action (reduces accidental inputs)

### Using the Button
- **Short press** (<1 second): Select / Confirm current action
- **Long press** (≥1 second): Go to Settings screen, or back to Main from Settings

### Using Touch
- **Tap** on keypad buttons to enter digits
- **Tap** on alarm status (main screen) to arm/disarm
- **Tap** anywhere on sensors screen to go back

---

## Screens

### Main Screen

The main screen is the home view showing alarm status and navigation.

**Display**:
- **Circular Menu**: KEYPAD, SENSORS, SETTINGS labels around the border
- **Alarm Status**: Large centered circle showing current state
  - ARMED (red): Alarm is armed
  - DISARMED (green): Alarm is disarmed
  - PENDING (yellow): Alarm is arming or in entry delay
  - TRIGGERED (red, flashing): Alarm has been triggered
- **Sensor Summary**: Shows "All closed" or "X open" below status

**Controls**:
- **Rotate encoder**: Highlight menu items (orange highlight)
- **Short press**: Go to highlighted screen
- **Touch alarm status**: 
  - If disarmed: Arms the alarm directly (no code required)
  - If armed: Goes to keypad to enter disarm code

---

### Keypad Screen

Phone-like numeric keypad for entering alarm codes.

**Layout**:
- **Code Display**: Shows entered digits as asterisks (masked)
- **Number Grid**: 3x3 grid for digits 1-9, with 0 at bottom center
- **CLR Button**: Left side, clears entered code
- **OK Button**: Right side, submits the code

**Controls**:
- **Rotate encoder**: Navigate between keys (highlighted in orange)
- **Short press**: Select highlighted key
- **Touch**: Tap any key directly

**Behavior**:
- When code is submitted, it sends ARM_AWAY or DISARM command via MQTT
- After submission, returns to main screen automatically

---

### Sensors Screen

Two-column list showing all configured door/window sensors.

**Display**:
- **Sensor List**: Each sensor shows:
  - Green dot: Sensor is closed
  - Red dot: Sensor is open
  - Sensor name (abbreviated to fit)

**Controls**:
- **Short press**: Return to main screen
- **Touch anywhere**: Return to main screen
- **Rotate encoder left**: Return to main screen

**Configured Sensors**:
The following sensors are monitored (configurable in code):
- Porta ingresso (Front door)
- PF cucina (Kitchen door)
- PF corridoio (Hallway door)
- Fin bagno PT (Ground floor bathroom window)
- Fin ingresso (Entrance window)
- Fin scale (Stairway window)
- PF camera (Bedroom door)
- Fin cam bimbe (Kids room window)
- Fin studio (Study window)
- Fin camera (Bedroom window)
- Fin bagno P1 (First floor bathroom window)

---

### Settings Screen

Configure the device's date and time.

**Display**:
- **Field List**: Hour, Minute, Day, Month, Year
- **Selection**: Orange highlight on selected field
- **Edit Mode**: Green highlight when editing a value

**Controls**:
- **Rotate encoder** (not editing): Navigate between fields
- **Rotate encoder** (editing): Adjust value up/down
- **Short press**: Toggle edit mode / Confirm value and move to next field
- **Long press**: Return to main screen

**Field Ranges**:
- Hour: 0-23
- Minute: 0-59
- Day: 1-31
- Month: 1-12
- Year: 2020-2099

---

## MQTT Integration

### Topics

The device uses the following MQTT topics:

| Topic | Purpose |
|-------|---------|
| `home/alarm/set` | Send commands (ARM_AWAY, DISARM) |
| `home/alarm` | Receive alarm state updates |
| `home/alarm/status` | Receive status updates |
| `home-assistant/<sensor>/contact` | Receive sensor states (ON=open, OFF=closed) |

### Alarm States

| State | Meaning |
|-------|---------|
| `armed_away` | Fully armed, away mode |
| `armed_home` | Armed, home mode |
| `armed_night` | Armed, night mode |
| `disarmed` | Not armed |
| `pending` | Entry/exit delay active |
| `arming` | Arming in progress |
| `triggered` | Alarm triggered |

---

## Troubleshooting

### WiFi Won't Connect

1. Verify SSID and password in `credentials.h`
2. Ensure WiFi network is 2.4GHz (ESP32-S3 doesn't support 5GHz)
3. Check that the router is within range
4. Connection timeout is 15 seconds - wait for full timeout

### MQTT Won't Connect

1. Verify MQTT server IP and port
2. Check MQTT username and password
3. Ensure MQTT broker is running and accessible
4. Common error codes:
   - -4: Connection timeout
   - -2: Connection failed
   - 4: Bad credentials
   - 5: Not authorized

### Time is Wrong

1. Ensure WiFi is connected (NTP sync requires internet)
2. Time zone is hardcoded to UTC+1 (CET) - modify `syncRtcFromNtp()` for other zones
3. Use Settings screen to manually adjust if needed

### Sensors Not Updating

1. Verify sensor MQTT topics match your Home Assistant configuration
2. Check that sensors are publishing to the expected topics
3. Monitor serial output (115200 baud) for incoming MQTT messages

### Display Flickering

1. This shouldn't happen - the display uses double-buffering
2. If it occurs, check for power supply issues
3. Try a different USB cable or power source

---

## Specifications

| Parameter | Value |
|-----------|-------|
| Display | 240x240 round TFT (GC9A01) |
| Processor | ESP32-S3 |
| Input | Rotary encoder + capacitive touch |
| Connectivity | WiFi 2.4GHz |
| Protocol | MQTT |
| RTC | Built-in BM8563 |
| Power | USB-C |

---

## Audio Feedback

The device provides audio feedback for user actions:
- **Short beep (2kHz, 20ms)**: Button press, touch, selection
- **Medium beep (1kHz, 50ms)**: Long press detected
- **Long beep (1kHz, 100ms)**: Command sent successfully
- **Click (4kHz, 10ms)**: Encoder rotation
