# M5StickC Plus Tricorder - User Manual

## Getting Started

### First Boot

1. Connect your M5StickC Plus with the Encoder Hat attached
2. Connect any sensors you want to use
3. Power on the device
4. The device will automatically detect connected sensors and start the appropriate app

### Understanding the Display

#### Header Bar
The top of every screen shows:
- **Date & Time**: MM/DD HH:MM format
- **Battery Time**: Estimated remaining runtime (when not charging)
- **Battery Icon**: 3-bar indicator with color coding
  - Green: >60%
  - Yellow: 20-60%
  - Red: <20%
- **Charging Indicator**: "+" symbol when connected to power
---

## Multi-Modal Alerts

When an alarm is triggered (Timer, Motion, 3D Level), the device uses multiple alert methods:

1. **Buzzer**: Plays the Imperial March melody
2. **Internal LED**: Flashes red LED on GPIO 10

---

## Navigation

### Opening the Menu
- **Long press** the front button (Button A), OR
- **Long press** the encoder button

### Menu Navigation
- **Rotate encoder** or **press side button** to move through apps
- Apps are shown in different colors:
  - **Bright**: Available (sensor detected)
  - **Dim gray**: Not available (sensor not connected)
- Status shows "Available" or "Not detected" for selected app

### Selecting an App
- **Short press** front button or encoder button
- Only available apps can be selected

---

## Apps

### Air Quality Monitor

**Sensor Required**: SCD4x CO2 Sensor and/or ENV Pro Unit (BME688)

**Display**:
- **CO2 level** (ppm) with color coding (if SCD4x connected):
  - Green: <1000 ppm (good)
  - Yellow: 1000-2000 ppm (moderate)
  - Red: >2000 ppm (poor)
- **IAQ (Indoor Air Quality)** index with color coding (if ENV Pro connected):
  - Green: 0-50 (excellent)
  - Yellow: 51-100 (good)
  - Orange: 101-150 (lightly polluted)
  - Red: 151-200 (moderately polluted)
  - Purple: 201-300 (heavily polluted)
  - Maroon: >300 (severely polluted)
  - Accuracy indicator (*0, *1, *2) shown while sensor is calibrating
- **Temperature** and **humidity** readings (from SCD4x if available, otherwise from ENV Pro)
- **Pressure** in hPa (if ENV Pro connected)
- **Gas resistance** in kOhm (if ENV Pro connected)
- 24-hour history graph showing temperature, humidity, and CO2 (if available)
- Min/Max temperature with timestamps

**Sensor Notes**:
- The ENV Pro uses the Bosch BSEC2 library for IAQ calculations
- IAQ accuracy improves over time as the sensor calibrates (24h for full accuracy)
- Both sensors can be used simultaneously for comprehensive air quality monitoring
- If only ENV Pro is connected, temperature and humidity are sourced from it

**Controls**:
- Short press (A/encoder): Arm/disarm air quality alarm
- Long press (A): Open menu
- Long press (B): Clear history data

**Alarm Mode**:
When armed, the alarm triggers if air quality becomes "bad":
- **CO2 threshold**: >1500 ppm (poor ventilation per ASHRAE guidelines)
- **IAQ threshold**: >150 (moderately polluted per Bosch BSEC)

The alarm uses all alert methods (buzzer melody, LED flash, Mi Band vibration) and automatically stops when air quality improves.

---

### Stopwatch

**Sensor Required**: None (always available)

**Display**:
- Elapsed time in HH:MM:SS.mmm format
- Running/Stopped status

**Controls**:
- Short press (front/encoder): Start/Stop
- Side button: Reset (when stopped)
- Long press: Open menu

---

### Timer (Countdown)

**Sensor Required**: None (always available)

**States**:
1. **Setting**: Adjust countdown time
2. **Running**: Timer counting down
3. **Paused**: Timer paused
4. **Finished**: Alarm playing

**Display**:
- Remaining time in MM:SS format
- Progress bar
- Current state indicator

**Controls**:

*Setting Mode*:
- Rotate encoder: Adjust time (variable step size)
- Short press: Start countdown

*Running Mode*:
- Short press: Pause
- Side button: Reset

*Paused Mode*:
- Short press: Resume
- Side button: Reset

*Finished Mode*:
- Short press: Stop alarm and reset

**Alarm**: Plays Pacman theme melody until dismissed

---

### Motion Alarm

**Sensor Required**: PIR Motion Sensor (Grove port)

**Display**:
- Status: ARMED/DISARMED/MOTION!
- PIR sensor state (HIGH/LOW)

**Controls**:
- Short press: Arm/Disarm the alarm
- Long press: Open menu

**Behavior**:
- When armed, plays alarm melody if motion detected
- Motion indicator stays red for 3 seconds after detection
- Disarming stops the alarm

---

### NCIR Temperature

**Sensor Required**: NCIR Hat (MLX90614)

**Display**:
- Object temperature (large, color-coded):
  - Red: >37.5°C (fever warning)
  - Green: 35-37.5°C (normal body temp)
  - Cyan: <35°C (cool)
  - Blue: <0°C (freezing)
- Ambient temperature
- Temperature bar (-70°C to 400°C range)

**Controls**:
- Long press: Open menu

**Usage Tips**:
- Hold sensor 1-3 cm from target surface
- Works on skin, objects, liquids
- Full range: -70°C to 382.2°C

---

### Heart Rate Monitor

**Sensor Required**: Heart Rate Hat (MAX30102)

**Display**:
- Current BPM (large, color-coded):
  - Red: >100 BPM (high)
  - Green: 60-100 BPM (normal)
  - Yellow: <60 BPM (low)
- Min/Max BPM with timestamps
- 12-hour history graph

**Controls**:
- Long press: Open menu

**Usage Tips**:
- Place fingertip firmly on sensor
- Keep finger still for accurate reading
- Wait a few seconds for reading to stabilize
- History records every 5 minutes when finger detected

---

### IR Remote

**Sensor Required**: IR Receiver (GPIO 36) + Built-in IR LED (GPIO 9)

**Display**:
- List of saved IR commands with protocol type
- "+ Learn New" option at bottom of list
- Learning mode indicator when capturing

**Controls**:

*List Mode*:
- Rotate encoder: Navigate command list
- Short press (A/encoder): Send selected command OR start learning
- Side button (B): Delete selected command
- Long press: Open menu

*Learning Mode*:
- Point any IR remote at the receiver
- Press a button on the remote to capture
- Command is automatically saved with default name
- Short press (A): Cancel learning

**Features**:
- Stores up to 10 IR commands in flash memory
- Supports NEC, Sony, RC5, RC6, Samsung, LG, Panasonic protocols
- Commands persist across reboots
- Shows protocol type for each saved command

**Usage Tips**:
- Connect an IR receiver module to GPIO 36 for learning
- Built-in IR LED on GPIO 9 is used for transmitting
- Point device at target when sending commands
- Works with most common IR remote protocols

---

### 3D Level

**Sensor Required**: Built-in MPU6886 IMU (no external hardware needed)

**Display**:
- Graphical bubble level indicator (pitch/roll)
- Numeric display of Pitch, Roll, and Yaw angles
- Color-coded bubble: Green (level), Yellow (close), Red (off level)
- Alarm status and reference angles when armed

**Controls**:
- Rotate encoder: Adjust alarm threshold (1° to 45°)
- Short press (A/encoder): Arm/disarm angle alarm
- Side button (B): Stop alarm when triggered
- Long press: Open menu

**Alarm Feature**:
- Press A to arm: captures current angles as reference
- Alarm triggers when any angle changes beyond threshold
- Plays melody until stopped with B button or disarmed

**Usage Tips**:
- Place device on surface to measure levelness
- Green bubble in center = perfectly level
- Yaw drifts over time (no magnetometer)
- Use for monitoring if something has been moved

---

### Distance Measurement

**Sensor Required**: TOF Hat (VL53L0X laser distance sensor)

**Display**:
- Distance in millimeters (large, color-coded):
  - Red: <100mm (very close)
  - Yellow: 100-300mm (close)
  - Green: 300-1000mm (medium)
  - Cyan: >1000mm (far)
- Distance also shown in cm and inches
- Visual bar graph (0-2000mm range)
- "OOR" displayed when out of range (>8190mm)

**Controls**:
- Long press: Open menu

**Usage Tips**:
- Sensor range: ~30mm to 2000mm (2 meters)
- Works best on flat, reflective surfaces
- May have reduced accuracy on dark or angled surfaces
- Updates continuously for real-time measurement

**Note**: TOF Hat uses the same port as Encoder Hat - they cannot be used simultaneously.

---

### TOF Counter

**Sensor Required**: TOF Hat (VL53L0X laser distance sensor)

**Display**:
- Object count (large number)
- Current distance reading
- Status: OBJECT (red) or CLEAR (green)
- Near/Far threshold values
- Visual bar with threshold markers and distance indicator

**Controls**:
- Short press (A): Reset counter to zero
- Side button (B): Cycle through threshold presets (100/200/300/500mm)
- Long press: Open menu

**How It Works**:
1. When an object comes closer than the "Near" threshold, it's detected
2. When the object moves away past the "Far" threshold, the count increments
3. This hysteresis prevents false counts from objects hovering at the threshold

**Threshold Presets**:
- 100mm near / 300mm far
- 200mm near / 400mm far
- 300mm near / 500mm far
- 500mm near / 700mm far

**Usage Tips**:
- Position sensor to face the path where objects will pass
- Adjust threshold based on expected object distance
- Works well for counting people, items on conveyor, etc.
- Counter persists until manually reset

**Note**: TOF Hat uses the same port as Encoder Hat - they cannot be used simultaneously.

---

### Sun & Moon

**Sensor Required**: None (always available)

**Display**:
- **Sun section**:
  - Dawn (civil twilight start)
  - Sunrise time
  - Sunset time
  - Dusk (civil twilight end)
  - Daylight duration (hours and minutes)
  - Tomorrow's daylight difference (green if gaining, red if losing)
- **Moon section**:
  - Moon phase name (New Moon, Waxing Crescent, etc.)
  - Illumination percentage
  - Approximate moonrise/moonset times (marked with ~)
- Location and date at bottom

**Controls**:
- Long press: Open menu

**Notes**:
- Location is set to Brescia, Italy (hardcoded)
- Sun times are calculated using the SunSet library
- Moon phase is calculated algorithmically
- Moonrise/moonset times are approximate

---

### Scale (Weight Measurement)

**Sensor Required**: MiniScale Unit (HX711-based load cell)

**Display**:
- Weight in grams (large, color-coded):
  - Yellow: Negative (needs tare)
  - Green: <10g (light)
  - Cyan: 10-100g (medium)
  - Orange: >100g (heavy)
- Weight also shown in kg, oz, and lb
- Raw ADC value (for debugging)
- Current gap/calibration value

**Controls**:
- Short press (A): Tare (zero the scale)
- Short press (B): Increase gap value (+0.1)
- Long press (B): Decrease gap value (-0.1)
- Long press (A): Open menu

**Tare Function**:
- Press A with empty scale to set zero point
- LED flashes red briefly to confirm
- Use before each weighing session

**Calibration (Gap Value)**:
- Gap value adjusts the weight calculation
- Increase if readings are too low
- Decrease if readings are too high
- Use a known weight to calibrate

**Usage Tips**:
- Place scale on flat, stable surface
- Tare before placing items
- Wait for reading to stabilize
- Avoid touching scale during measurement

---

### Home Assistant

**Sensor Required**: None (always available)

Connects to your WiFi network and sends commands to Home Assistant to trigger an alarm.

**Configuration** (edit in code):
- `HA_WIFI_SSID`: Your WiFi network name
- `HA_WIFI_PASSWORD`: Your WiFi password (WPA2)
- `HA_IP`: Home Assistant server IP address
- `HA_PORT`: Home Assistant port (default 8123)
- `HA_API_TOKEN`: Long-lived access token from Home Assistant

**Display**:
- WiFi connection status (Disconnected/Connecting/Connected)
- SSID and IP address when connected
- Home Assistant server address
- Action buttons

**Controls**:
- Short press (A/encoder): Connect to WiFi / Cancel connection / Trigger alarm
- Side button (B): Disconnect from WiFi
- Long press: Open menu

**States**:
1. **Disconnected**: Press A to connect to WiFi
2. **Connecting**: Animated dots, press A to cancel
3. **Connected**: Press A to trigger the alarm, B to disconnect
4. **Success/Error**: Shows result for 2 seconds, then returns to connected/disconnected

**Getting a Home Assistant API Token**:
1. In Home Assistant, go to your Profile (bottom left)
2. Scroll to "Long-Lived Access Tokens"
3. Click "Create Token" and copy it to the code

**Note**: The alarm entity is hardcoded as `alarm_control_panel.alarmo`. Edit `haSendAlarmCommand()` to change the entity or service.

---

### Settings

**Sensor Required**: None (always available)

Allows adjusting the device's date and time via the RTC (Real-Time Clock).

**Display**:
- Five editable fields: Hour, Minute, Day, Month, Year
- Selected field is highlighted
- Current field value shown in edit mode

**Controls**:
- Rotate encoder: Navigate between fields / Adjust value when editing
- Short press (A/encoder): Enter edit mode / Confirm value
- Side button (B): Navigate to next field / Increment value when editing
- Long press: Open menu

**Usage Tips**:
- Time is stored in the RTC and persists across reboots
- Use NTP sync at boot for automatic time setting (requires WiFi)

---

## Troubleshooting

### Sensor Not Detected

1. Check physical connection
2. Verify correct port (I2C vs Grove)
3. Power cycle the device
4. Check serial monitor for detection messages

### Heart Rate Not Reading

1. Ensure firm, steady finger pressure
2. Wait for "Finger detected" message
3. Keep finger still for 5-10 seconds
4. Try different finger position

### No Sound from Buzzer

1. Buzzer uses GPIO 2
2. Check for hardware issues
3. Volume is fixed in firmware

### IR Remote Not Learning

1. Ensure IR receiver is connected to GPIO 36
2. Point remote directly at receiver
3. Try pressing remote button multiple times
4. Check serial monitor for debug output

### IR Commands Not Sending

1. Built-in IR LED is on GPIO 9
2. Point device at target device
3. Try from closer distance
4. Some devices require specific timing

### Battery Drains Quickly

- Estimated runtime based on 120mAh battery at 40mA average
- Actual runtime varies with sensor usage
- Heart rate sensor uses more power when active

---

## Specifications

| Parameter | Value |
|-----------|-------|
| Display | 135x240 TFT |
| Processor | ESP32-PICO-D4 |
| Battery | 120mAh LiPo |
| I2C Speed | 100-400 kHz |
| Sample Rates | 5-10 min intervals |
| History Duration | 12-24 hours |
