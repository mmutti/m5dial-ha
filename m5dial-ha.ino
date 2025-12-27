/*
 * M5Dial Home Assistant Controller
 * 
 * Controls Home Assistant alarm via MQTT using the M5Dial's rotary encoder
 * and touch screen with a phone-like keypad interface.
 * 
 * Features:
 * - WiFi connection at startup
 * - MQTT connection to Home Assistant
 * - Rotary encoder for navigation and code input
 * - Touch screen keypad for code input
 * - RTC time display and configuration
 * - Status display showing WiFi and MQTT connection state
 * 
 * Based on M5Dial library and adapted from m5stickc-tricorder Home Assistant app
 */

#include "M5Dial.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

// Credentials - copy credentials.h.example to credentials.h and fill in your values
#include "credentials.h"

// MQTT Topics for Home Assistant alarm
#define HA_MQTT_COMMAND_TOPIC "home/alarm/set"
#define HA_MQTT_STATE_TOPIC "home/alarm"
#define HA_MQTT_STATUS_TOPIC "home/alarm/status"

// Binary sensors for alarm (doors/windows)
#define HA_SENSOR_COUNT 11
const char* haSensorTopics[HA_SENSOR_COUNT] = {
    "home-assistant/porta_ingresso/contact",
    "home-assistant/portafinestra_cucina/contact",
    "home-assistant/portafinestra_corridoio/contact",
    "home-assistant/finestra_bagno_pt/contact",
    "home-assistant/finestra_ingresso/contact",
    "home-assistant/finestra_scale/contact",
    "home-assistant/portafinestra_camera/contact",
    "home-assistant/finestra_camera_bimbe/contact",
    "home-assistant/finestra_studio/contact",
    "home-assistant/finestra_camera/contact",
    "home-assistant/finestra_bagno_p1/contact"
};
const char* haSensorNames[HA_SENSOR_COUNT] = {
    "Porta ingresso",
    "PF cucina",
    "PF corridoio",
    "Fin bagno PT",
    "Fin ingresso",
    "Fin scale",
    "PF camera",
    "Fin cam bimbe",
    "Fin studio",
    "Fin camera",
    "Fin bagno P1"
};

// Display constants for round 240x240 display
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define CENTER_X 120
#define CENTER_Y 120

// Sprite for double-buffering (prevents flickering)
M5Canvas canvas(&M5Dial.Display);

// Menu options count for circular border (excludes MAIN/ALARM since that's the current screen)
#define MENU_COUNT 3
const char* menuNames[MENU_COUNT] = {"KEYPAD", "SENSORS", "SETTINGS"};
int menuSelection = 0;  // Currently highlighted menu item (0=KEYPAD, 1=SENSORS, 2=SETTINGS)

// Colors
#define COLOR_BG        0x0000  // Black
#define COLOR_TEXT      0xFFFF  // White
#define COLOR_ACCENT    0x07FF  // Cyan
#define COLOR_OK        0x03E0  // Dark green
#define COLOR_WARN      0xFFE0  // Yellow
#define COLOR_ERROR     0xF800  // Red
#define COLOR_DIM       0x7BEF  // Gray
#define COLOR_ARMED     0xF800  // Red
#define COLOR_DISARMED  0x03E0  // Dark green
#define COLOR_ORANGE    0xC260  // Dark orange for menu highlight

// App states
enum AppScreen {
    SCREEN_MAIN,      // Main status screen
    SCREEN_KEYPAD,    // Keypad for code entry
    SCREEN_SENSORS,   // Sensor status list
    SCREEN_SETTINGS   // RTC settings
};

// Menu screens mapping (must be after AppScreen enum)
const AppScreen menuScreens[MENU_COUNT] = {SCREEN_KEYPAD, SCREEN_SENSORS, SCREEN_SETTINGS};

// Settings fields
enum SettingsField { SET_HOUR, SET_MIN, SET_DAY, SET_MONTH, SET_YEAR, SET_FIELD_COUNT };

// Connection states
enum ConnState { CONN_DISCONNECTED, CONN_CONNECTING, CONN_CONNECTED, CONN_ERROR };

// Global state
AppScreen currentScreen = SCREEN_MAIN;
AppScreen previousScreen = SCREEN_MAIN;  // Track screen changes for redraw optimization
SettingsField settingsField = SET_HOUR;
bool settingsEditing = false;
bool needsRedraw = true;  // Force redraw when state changes
ConnState wifiState = CONN_DISCONNECTED;
ConnState mqttState = CONN_DISCONNECTED;
String haAlarmState = "unknown";
bool haSensorOpen[HA_SENSOR_COUNT] = {false};
String lastError = "";

// Code entry
#define CODE_MAX_LEN 8
char enteredCode[CODE_MAX_LEN + 1] = "";
int codeLength = 0;
int selectedKey = 5;  // Start at '5' (center of keypad)

// Encoder tracking
long lastEncoderValue = 0;
int encoderAccumulator = 0;

// Touch keypad layout: 3x3 number grid with CLR on left and OK on right as tall buttons
// Keys: 1,2,3 / 4,5,6 / 7,8,9 / 0 (center bottom)
// CLR = index 10, OK = index 11 (special tall buttons on sides)
const char keypadCharsDefault[12] = {'1','2','3','4','5','6','7','8','9','0','C','>'};
char keypadChars[12] = {'1','2','3','4','5','6','7','8','9','0','C','>'};
int keypadMapping[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};  // Maps position to digit index
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
#define KEY_WIDTH 50
#define KEY_HEIGHT 38
#define KEYPAD_START_X 45  // Centered: (240 - 3*50) / 2 = 45
#define KEYPAD_START_Y 70
#define SIDE_BTN_WIDTH 15  // Half of previous width
#define SIDE_BTN_HEIGHT (KEY_HEIGHT * 3 + 6)  // Height of 3 buttons

// MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Preferences prefs;

// Timing
unsigned long lastReconnectAttempt = 0;
unsigned long wifiConnectStart = 0;
#define WIFI_TIMEOUT_MS 15000
#define MQTT_RECONNECT_INTERVAL 5000

// NTP sync state
bool ntpSynced = false;

// Animation state
unsigned long animationStartTime = 0;

// Alarm buzzer state
bool alarmBuzzerActive = false;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

// Long press detection
#define LONG_PRESS_MS 1000
unsigned long btnPressStart = 0;
bool btnWasPressed = false;
bool btnLongTriggered = false;

// Forward declarations
void drawMainScreen();
void drawKeypadScreen();
void drawSensorsScreen();
void drawSettingsScreen();
void drawStatusBar();
void drawWifiIcon(int x, int y, bool connected);
void handleTouch();
void handleEncoder();
void connectWiFi();
void connectMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setRtcFromCompileTime();
void syncRtcFromNtp();
void shuffleKeypad();
void playAlarmBuzzer();
void stopAlarmBuzzer();

void setup() {
    Serial.begin(115200);
    Serial.println("M5Dial Home Assistant Controller");
    
    // Initialize M5Dial with encoder enabled
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);  // encoder=true, RFID=false
    
    // Configure display
    M5Dial.Display.setRotation(0);
    M5Dial.Display.fillScreen(COLOR_BG);
    
    // Create sprite for double-buffering
    canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    
    // Show startup message
    canvas.fillScreen(COLOR_BG);
    canvas.drawString("M5Dial HA", CENTER_X, CENTER_Y - 20);
    canvas.drawString("Connecting...", CENTER_X, CENTER_Y + 20);
    canvas.pushSprite(0, 0);
    
    // Initialize preferences for RTC sync
    prefs.begin("m5dial-ha", false);
    
    // Set RTC from compile time if not already set
    setRtcFromCompileTime();
    
    // Initialize encoder
    lastEncoderValue = M5Dial.Encoder.read();
    
    // Start WiFi connection
    connectWiFi();
    
    // Setup MQTT
    mqttClient.setServer(HA_MQTT_SERVER, HA_MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(512);
}

void loop() {
    M5Dial.update();
    
    // Handle WiFi connection
    if (wifiState == CONN_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            wifiState = CONN_CONNECTED;
            Serial.println("WiFi connected!");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            
            // Sync RTC from NTP (will be checked in loop)
            // Use 0 offset - getLocalTime returns UTC, we add offset in syncRtcFromNtp
            configTime(0, 0, "pool.ntp.org", "time.google.com");
            ntpSynced = false;
            
            // Connect to MQTT
            connectMqtt();
        } else if (millis() - wifiConnectStart > WIFI_TIMEOUT_MS) {
            wifiState = CONN_ERROR;
            lastError = "WiFi timeout";
            Serial.println("WiFi connection timeout");
        }
    }
    
    // Handle MQTT connection
    if (wifiState == CONN_CONNECTED) {
        if (!mqttClient.connected()) {
            mqttState = CONN_DISCONNECTED;
            if (millis() - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
                lastReconnectAttempt = millis();
                connectMqtt();
            }
        } else {
            mqttState = CONN_CONNECTED;
            mqttClient.loop();
        }
        
        // Check if NTP time is available and sync RTC
        if (!ntpSynced) {
            syncRtcFromNtp();
        }
    }
    
    // Handle alarm buzzer
    if (haAlarmState == "triggered") {
        if (!alarmBuzzerActive) {
            alarmBuzzerActive = true;
            lastBuzzerToggle = millis();
            buzzerState = true;
        }
        playAlarmBuzzer();
    } else {
        if (alarmBuzzerActive) {
            stopAlarmBuzzer();
        }
    }
    
    // Handle input
    handleEncoder();
    handleTouch();
    handleButton();
    
    // Draw current screen (only when needed or state changed)
    switch (currentScreen) {
        case SCREEN_MAIN:
            drawMainScreen();
            break;
        case SCREEN_KEYPAD:
            drawKeypadScreen();
            break;
        case SCREEN_SENSORS:
            drawSensorsScreen();
            break;
        case SCREEN_SETTINGS:
            drawSettingsScreen();
            break;
    }
    
    // Push sprite to display
    canvas.pushSprite(0, 0);
    
    delay(50);  // Reduce update rate to prevent flicker
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(HA_WIFI_SSID, HA_WIFI_PASSWORD);
    wifiState = CONN_CONNECTING;
    wifiConnectStart = millis();
    Serial.printf("Connecting to WiFi: %s\n", HA_WIFI_SSID);
}

void connectMqtt() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    mqttState = CONN_CONNECTING;
    Serial.printf("Connecting to MQTT: %s:%d\n", HA_MQTT_SERVER, HA_MQTT_PORT);
    
    if (mqttClient.connect("m5dial-ha", HA_MQTT_USER, HA_MQTT_PASSWORD)) {
        mqttState = CONN_CONNECTED;
        Serial.println("MQTT connected!");
        
        // Subscribe to topics
        mqttClient.subscribe(HA_MQTT_STATE_TOPIC);
        mqttClient.subscribe(HA_MQTT_STATUS_TOPIC);
        for (int i = 0; i < HA_SENSOR_COUNT; i++) {
            mqttClient.subscribe(haSensorTopics[i]);
        }
    } else {
        mqttState = CONN_ERROR;
        int rc = mqttClient.state();
        Serial.printf("MQTT failed, rc=%d\n", rc);
        switch(rc) {
            case -4: lastError = "MQTT timeout"; break;
            case -2: lastError = "MQTT conn fail"; break;
            case 4: lastError = "MQTT bad creds"; break;
            case 5: lastError = "MQTT unauth"; break;
            default: lastError = "MQTT error"; break;
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.printf("MQTT [%s]: %s\n", topic, message.c_str());
    
    String topicStr = String(topic);
    
    if (topicStr == HA_MQTT_STATE_TOPIC) {
        haAlarmState = message;
    }
    
    // Check sensor topics
    for (int i = 0; i < HA_SENSOR_COUNT; i++) {
        if (topicStr == haSensorTopics[i]) {
            haSensorOpen[i] = (message == "ON" || message == "on" || message == "1" || message == "true");
            break;
        }
    }
}

void handleButton() {
    unsigned long now = millis();
    bool btnShortPress = false;
    bool btnLongPress = false;
    
    if (M5Dial.BtnA.isPressed()) {
        if (!btnWasPressed) {
            btnWasPressed = true;
            btnPressStart = now;
            btnLongTriggered = false;
        } else if (!btnLongTriggered && now - btnPressStart >= LONG_PRESS_MS) {
            btnLongPress = true;
            btnLongTriggered = true;
        }
    } else {
        if (btnWasPressed && !btnLongTriggered && (now - btnPressStart < LONG_PRESS_MS)) {
            btnShortPress = true;
        }
        btnWasPressed = false;
        btnLongTriggered = false;
    }
    
    // Handle button actions based on screen
    if (btnLongPress) {
        // Long press: go to settings from any screen, or back to main from settings
        if (currentScreen == SCREEN_SETTINGS) {
            currentScreen = SCREEN_MAIN;
            settingsEditing = false;
        } else {
            currentScreen = SCREEN_SETTINGS;
            settingsField = SET_HOUR;
            settingsEditing = false;
        }
        M5Dial.Speaker.tone(1000, 50);
    }
    
    if (btnShortPress) {
        M5Dial.Speaker.tone(2000, 20);
        
        switch (currentScreen) {
            case SCREEN_MAIN:
                // Short press on main: go to selected screen from menu
                currentScreen = menuScreens[menuSelection];
                if (currentScreen == SCREEN_KEYPAD) {
                    codeLength = 0;
                    enteredCode[0] = '\0';
                    selectedKey = 5;
                    shuffleKeypad();
                }
                break;
                
            case SCREEN_KEYPAD:
                // Short press on keypad: select current key
                handleKeypadSelect();
                break;
                
            case SCREEN_SENSORS:
                // Short press on sensors: back to main
                currentScreen = SCREEN_MAIN;
                break;
                
            case SCREEN_SETTINGS:
                // Short press on settings: toggle edit mode or confirm
                if (settingsEditing) {
                    // Confirm edit, move to next field
                    settingsEditing = false;
                    settingsField = (SettingsField)((settingsField + 1) % SET_FIELD_COUNT);
                } else {
                    // Enter edit mode
                    settingsEditing = true;
                }
                break;
        }
    }
}

void handleEncoder() {
    long newValue = M5Dial.Encoder.read();
    long delta = newValue - lastEncoderValue;
    
    if (delta == 0) return;
    
    lastEncoderValue = newValue;
    encoderAccumulator += delta;
    
    // Require 2 steps for action (reduces sensitivity)
    if (abs(encoderAccumulator) < 2) return;
    
    int direction = (encoderAccumulator > 0) ? 1 : -1;
    encoderAccumulator = 0;
    
    M5Dial.Speaker.tone(4000, 10);
    
    switch (currentScreen) {
        case SCREEN_MAIN:
            // Encoder on main: cycle through menu selection (shown on border)
            menuSelection = (menuSelection + direction + MENU_COUNT) % MENU_COUNT;
            break;
            
        case SCREEN_KEYPAD:
            // Encoder on keypad: navigate keys
            selectedKey = (selectedKey + direction + 12) % 12;
            break;
            
        case SCREEN_SENSORS:
            // Encoder on sensors: scroll (if needed) or go back
            if (direction < 0) {
                currentScreen = SCREEN_MAIN;
            }
            break;
            
        case SCREEN_SETTINGS:
            if (settingsEditing) {
                // Adjust current field value
                adjustSettingsField(direction);
            } else {
                // Navigate fields
                settingsField = (SettingsField)((settingsField + direction + SET_FIELD_COUNT) % SET_FIELD_COUNT);
            }
            break;
    }
}

void handleTouch() {
    auto touch = M5Dial.Touch.getDetail();
    
    if (!touch.wasPressed()) return;
    
    int tx = touch.x;
    int ty = touch.y;
    
    if (currentScreen == SCREEN_KEYPAD) {
        // Check if touch is on CLR button (left side)
        if (tx >= 10 && tx < 10 + SIDE_BTN_WIDTH + 10 &&
            ty >= KEYPAD_START_Y && ty < KEYPAD_START_Y + SIDE_BTN_HEIGHT) {
            selectedKey = 10;  // CLR
            handleKeypadSelect();
            M5Dial.Speaker.tone(2000, 20);
        }
        // Check if touch is on OK button (right side)
        else if (tx >= 190 && tx < 230 &&
                 ty >= KEYPAD_START_Y && ty < KEYPAD_START_Y + SIDE_BTN_HEIGHT) {
            selectedKey = 11;  // OK
            handleKeypadSelect();
            M5Dial.Speaker.tone(2000, 20);
        }
        // Check if touch is on number keypad (3x3 grid + 0)
        else if (tx >= KEYPAD_START_X && tx < KEYPAD_START_X + KEYPAD_COLS * KEY_WIDTH &&
                 ty >= KEYPAD_START_Y && ty < KEYPAD_START_Y + KEYPAD_ROWS * KEY_HEIGHT) {
            
            int col = (tx - KEYPAD_START_X) / KEY_WIDTH;
            int row = (ty - KEYPAD_START_Y) / KEY_HEIGHT;
            
            int keyIndex = -1;
            if (row < 3) {
                // Keys 1-9
                keyIndex = row * 3 + col;
            } else if (row == 3 && col == 1) {
                // Key 0 (center of bottom row)
                keyIndex = 9;
            }
            
            if (keyIndex >= 0 && keyIndex < 10) {
                selectedKey = keyIndex;
                handleKeypadSelect();
                M5Dial.Speaker.tone(2000, 20);
            }
        }
    } else if (currentScreen == SCREEN_MAIN) {
        // Touch on main screen: arm directly, but require code to disarm
        bool isArmed = (haAlarmState == "armed_away" || haAlarmState == "armed_home" || 
                        haAlarmState == "armed_night" || haAlarmState == "arming" ||
                        haAlarmState == "pending" || haAlarmState == "triggered");
        
        if (isArmed) {
            // Alarm is armed - go to keypad to enter code for disarming
            currentScreen = SCREEN_KEYPAD;
            codeLength = 0;
            enteredCode[0] = '\0';
            selectedKey = 5;
            shuffleKeypad();
        } else {
            // Alarm is disarmed - arm directly without code
            if (mqttClient.connected()) {
                mqttClient.publish(HA_MQTT_COMMAND_TOPIC, "ARM_AWAY");
                Serial.println("Sent: ARM_AWAY");
            }
        }
        M5Dial.Speaker.tone(2000, 20);
    } else if (currentScreen == SCREEN_SENSORS) {
        // Touch anywhere to go back
        currentScreen = SCREEN_MAIN;
        M5Dial.Speaker.tone(2000, 20);
    }
}

void handleKeypadSelect() {
    char key = keypadChars[selectedKey];
    
    if (key == 'C') {
        // Clear
        codeLength = 0;
        enteredCode[0] = '\0';
    } else if (key == '>') {
        // Submit code
        if (codeLength > 0) {
            submitCode();
        }
    } else {
        // Add digit
        if (codeLength < CODE_MAX_LEN) {
            enteredCode[codeLength++] = key;
            enteredCode[codeLength] = '\0';
        }
    }
}

void submitCode() {
    // Send the code to Home Assistant
    // The code is typically used for arming/disarming
    bool isArmed = (haAlarmState == "armed_away" || haAlarmState == "armed_home" || 
                    haAlarmState == "armed_night" || haAlarmState == "triggered");
    
    if (isArmed) {
        // Validate PIN before disarming
        if (strcmp(enteredCode, HA_ALARM_PIN) != 0) {
            // Wrong PIN - play error tone and clear
            Serial.printf("Wrong PIN entered: %s\n", enteredCode);
            M5Dial.Speaker.tone(200, 500);  // Low error tone
            codeLength = 0;
            enteredCode[0] = '\0';
            return;  // Stay on keypad screen
        }
    }
    
    if (mqttClient.connected()) {
        // Build command with code
        String command = isArmed ? "DISARM" : "ARM_AWAY";
        if (mqttClient.publish(HA_MQTT_COMMAND_TOPIC, command.c_str())) {
            Serial.printf("Sent: %s (code: %s)\n", command.c_str(), enteredCode);
            M5Dial.Speaker.tone(1000, 100);  // Success tone
        }
    }
    
    // Clear code and go back to main
    codeLength = 0;
    enteredCode[0] = '\0';
    currentScreen = SCREEN_MAIN;
}

void toggleAlarm() {
    bool isArmed = (haAlarmState == "armed_away" || haAlarmState == "armed_home" || 
                    haAlarmState == "armed_night" || haAlarmState == "arming" ||
                    haAlarmState == "pending");
    
    if (mqttClient.connected()) {
        const char* command = isArmed ? "DISARM" : "ARM_AWAY";
        if (mqttClient.publish(HA_MQTT_COMMAND_TOPIC, command)) {
            Serial.printf("Sent: %s\n", command);
            M5Dial.Speaker.tone(1000, 100);
        }
    }
}

void adjustSettingsField(int direction) {
    // Get current RTC time
    auto dt = M5Dial.Rtc.getDateTime();
    
    switch (settingsField) {
        case SET_HOUR:
            dt.time.hours = (dt.time.hours + direction + 24) % 24;
            break;
        case SET_MIN:
            dt.time.minutes = (dt.time.minutes + direction + 60) % 60;
            break;
        case SET_DAY:
            dt.date.date = ((dt.date.date - 1 + direction + 31) % 31) + 1;
            break;
        case SET_MONTH:
            dt.date.month = ((dt.date.month - 1 + direction + 12) % 12) + 1;
            break;
        case SET_YEAR:
            dt.date.year = constrain(dt.date.year + direction, 2020, 2099);
            break;
        default:
            break;
    }
    
    // Update RTC
    M5Dial.Rtc.setDateTime(dt);
}

// Draw WiFi icon (SVG-style wireless signal arcs) - white when connected, gray when not
void drawWifiIcon(int x, int y, bool connected) {
    uint16_t color = connected ? COLOR_TEXT : COLOR_DIM;  // White if connected, gray if not
    
    // Draw concentric arcs for WiFi symbol (SVG-style with thicker lines)
    // Small dot at bottom (the transmitter point)
    canvas.fillCircle(x, y + 8, 3, color);
    
    // Arc 1 (smallest) - draw with thickness
    for (int a = -40; a <= 40; a += 2) {
        float rad = a * PI / 180.0;
        int px = x + (int)(6 * sin(rad));
        int py = y + 5 - (int)(6 * cos(rad));
        canvas.fillCircle(px, py, 1, color);
    }
    
    // Arc 2 (medium)
    for (int a = -40; a <= 40; a += 2) {
        float rad = a * PI / 180.0;
        int px = x + (int)(10 * sin(rad));
        int py = y + 5 - (int)(10 * cos(rad));
        canvas.fillCircle(px, py, 1, color);
    }
    
    // Arc 3 (largest)
    for (int a = -40; a <= 40; a += 2) {
        float rad = a * PI / 180.0;
        int px = x + (int)(14 * sin(rad));
        int py = y + 5 - (int)(14 * cos(rad));
        canvas.fillCircle(px, py, 1, color);
    }
}

void drawStatusBar() {
    // Status indicators at top, beside the clock
    int y = 25;
    
    // WiFi icon at left of clock
    bool wifiOk = (wifiState == CONN_CONNECTED);
    drawWifiIcon(CENTER_X - 50, y, wifiOk);
    
    // Time at top center
    auto dt = M5Dial.Rtc.getDateTime();
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", dt.time.hours, dt.time.minutes);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(COLOR_TEXT);
    canvas.drawString(timeStr, CENTER_X, y);
    
    // MQTT label at right of clock
    canvas.setTextDatum(middle_center);
    if (mqttState == CONN_CONNECTED) {
        canvas.setTextColor(COLOR_TEXT);  // White when connected
    } else {
        canvas.setTextColor(COLOR_ERROR);  // Red when disconnected
    }
    canvas.drawString("MQTT", CENTER_X + 50, y);
}

// Draw text along circular arc (for menu items on border)
// Text is drawn character by character following the arc
// clockwise=true: text reads clockwise, clockwise=false: text reads anticlockwise
void drawTextOnArc(const char* text, float centerAngleDeg, int radius, uint16_t color, bool highlight, bool clockwise = true) {
    int len = strlen(text);
    float charSpacing = 8.0;  // Degrees between characters
    float totalSpan = (len - 1) * charSpacing;
    float startAngle;
    if (clockwise) {
        startAngle = centerAngleDeg - totalSpan / 2.0;
    } else {
        // Anticlockwise: start from the end
        startAngle = centerAngleDeg + totalSpan / 2.0;
    }
    
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    
    // Draw highlight arc segment behind text if selected
    if (highlight) {
        // Calculate arc bounds correctly for both clockwise and anticlockwise
        float arcStartAngle, arcEndAngle;
        if (clockwise) {
            arcStartAngle = startAngle - 5;
            arcEndAngle = startAngle + totalSpan + 5;
        } else {
            // For anticlockwise, startAngle is at the end of the word
            arcEndAngle = startAngle + 5;
            arcStartAngle = startAngle - totalSpan - 5;
        }
        float arcStart = arcStartAngle * PI / 180.0;
        float arcEnd = arcEndAngle * PI / 180.0;
        for (float a = arcStart; a <= arcEnd; a += 0.02) {
            int x = CENTER_X + (int)((radius) * sin(a));
            int y = CENTER_Y - (int)((radius) * cos(a));
            canvas.fillCircle(x, y, 8, COLOR_ORANGE);  // Orange highlight
        }
        canvas.setTextColor(COLOR_TEXT);  // White text on orange
    } else {
        canvas.setTextColor(color);
    }
    
    // Draw each character along the arc
    for (int i = 0; i < len; i++) {
        float angleDeg;
        if (clockwise) {
            angleDeg = startAngle + i * charSpacing;
        } else {
            angleDeg = startAngle - i * charSpacing;
        }
        float angleRad = angleDeg * PI / 180.0;
        int x = CENTER_X + (int)(radius * sin(angleRad));
        int y = CENTER_Y - (int)(radius * cos(angleRad));
        
        char ch[2] = {text[i], '\0'};
        canvas.drawString(ch, x, y);
    }
}

void drawMainScreen() {
    canvas.fillScreen(COLOR_BG);
    
    // Draw menu labels around the circular border (3 items: KEYPAD, SENSORS, SETTINGS)
    // KEYPAD at 270° (left side), SENSORS at 90° (right side), SETTINGS at 180° (bottom)
    // KEYPAD and SETTINGS are drawn anticlockwise so they read correctly
    float angles[MENU_COUNT] = {270, 90, 180};  // KEYPAD left, SENSORS right, SETTINGS bottom
    bool anticlockwiseFlags[MENU_COUNT] = {true, false, true};  // KEYPAD anticlockwise, SENSORS clockwise, SETTINGS anticlockwise
    for (int i = 0; i < MENU_COUNT; i++) {
        bool isSelected = (i == menuSelection);
        drawTextOnArc(menuNames[i], angles[i], 108, COLOR_DIM, isSelected, !anticlockwiseFlags[i]);
    }
    
    drawStatusBar();
    
    // Alarm state - large centered display
    canvas.setTextDatum(middle_center);
    
    // Draw circular background for alarm state
    uint16_t stateColor = COLOR_DIM;
    String stateText = haAlarmState;
    bool isArmedState = false;
    bool isTriggeredState = false;
    
    if (haAlarmState == "armed_away" || haAlarmState == "armed_home" || haAlarmState == "armed_night") {
        stateColor = COLOR_ARMED;
        stateText = "ARMED";
        isArmedState = true;
    } else if (haAlarmState == "disarmed") {
        stateColor = COLOR_DISARMED;
        stateText = "DISARMED";
    } else if (haAlarmState == "pending" || haAlarmState == "arming") {
        stateColor = COLOR_WARN;
        stateText = "PENDING";
    } else if (haAlarmState == "triggered") {
        stateColor = COLOR_ERROR;
        stateText = "TRIGGERED";
        isTriggeredState = true;
    }
    
    // Calculate animated color for circle only (not text)
    // Sleeping man breathing: ~12-20 breaths/min = 0.2-0.33 Hz, use 5 second cycle (0.2 Hz)
    // Running man breathing: ~40-60 breaths/min = 0.67-1 Hz, use 400ms cycle (2.5 Hz)
    uint16_t circleColor = stateColor;
    if (isArmedState || isTriggeredState) {
        unsigned long now = millis();
        float brightness;
        
        if (isTriggeredState) {
            // Fast pulsing for triggered state (400ms cycle = 2.5 Hz)
            float phase = (now % 400) / 400.0f;
            brightness = 0.5f + 0.5f * sin(phase * 2.0f * PI);
        } else {
            // Slow breathing for armed state (5000ms cycle = 0.2 Hz)
            float phase = (now % 5000) / 5000.0f;
            brightness = 0.5f + 0.5f * sin(phase * 2.0f * PI);
        }
        
        // Scale red component from 0 to full red (0xF800)
        uint8_t r = (uint8_t)(31 * brightness);  // 5 bits for red in RGB565
        circleColor = (r << 11);  // RGB565: RRRRR GGGGGG BBBBB
    }
    
    // Draw state circle (animated)
    canvas.drawCircle(CENTER_X, CENTER_Y, 60, circleColor);
    canvas.drawCircle(CENTER_X, CENTER_Y, 61, circleColor);
    
    // State text (static color, not animated)
    canvas.setTextColor(stateColor);
    canvas.setTextSize(2);
    canvas.drawString(stateText, CENTER_X, CENTER_Y);
    canvas.setTextSize(1);
    
    // Sensor summary below center
    int openCount = 0;
    String openSensorNames = "";
    for (int i = 0; i < HA_SENSOR_COUNT; i++) {
        if (haSensorOpen[i]) {
            openCount++;
            if (openSensorNames.length() > 0) {
                openSensorNames += ", ";
            }
            openSensorNames += haSensorNames[i];
        }
    }
    
    canvas.setTextDatum(middle_center);
    if (openCount == 0) {
        canvas.setTextColor(COLOR_OK);
        canvas.drawString("All closed", CENTER_X, CENTER_Y + 45);
    } else {
        canvas.setTextColor(COLOR_ERROR);
        // Show sensor names when triggered, otherwise show count
        if (isTriggeredState && openCount <= 2) {
            // Show names if 1-2 sensors open (fits on screen)
            canvas.drawString(openSensorNames, CENTER_X, CENTER_Y + 45);
        } else if (isTriggeredState) {
            // Too many open, show first name + count
            String firstOpen = "";
            for (int i = 0; i < HA_SENSOR_COUNT; i++) {
                if (haSensorOpen[i]) {
                    firstOpen = haSensorNames[i];
                    break;
                }
            }
            char buf[48];
            snprintf(buf, sizeof(buf), "%s +%d", firstOpen.c_str(), openCount - 1);
            canvas.drawString(buf, CENTER_X, CENTER_Y + 45);
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d open", openCount);
            canvas.drawString(buf, CENTER_X, CENTER_Y + 45);
        }
    }
}

void drawKeypadScreen() {
    canvas.fillScreen(COLOR_BG);
    
    drawStatusBar();
    
    // Title - only show "ENTER CODE" when no code entered
    canvas.setTextDatum(middle_center);
    if (codeLength == 0) {
        canvas.setTextColor(COLOR_ORANGE);
        canvas.drawString("ENTER CODE", CENTER_X, 48);
    }
    
    // Draw entered code (masked with asterisks)
    canvas.setTextSize(2);
    canvas.setTextColor(COLOR_TEXT);
    String maskedCode = "";
    for (int i = 0; i < codeLength; i++) {
        maskedCode += "*";
    }
    if (codeLength > 0) {
        canvas.drawString(maskedCode, CENTER_X, 48);
    }
    canvas.setTextSize(1);
    
    // Draw CLR button on left side (no border, just text)
    int clrX = 15;
    int clrY = KEYPAD_START_Y;
    bool clrSelected = (selectedKey == 10);
    if (clrSelected) {
        canvas.setTextColor(COLOR_ACCENT);
    } else {
        canvas.setTextColor(COLOR_DIM);
    }
    canvas.setTextDatum(middle_center);
    // Draw CLR vertically
    canvas.drawString("C", clrX, clrY + SIDE_BTN_HEIGHT/2 - 15);
    canvas.drawString("L", clrX, clrY + SIDE_BTN_HEIGHT/2);
    canvas.drawString("R", clrX, clrY + SIDE_BTN_HEIGHT/2 + 15);
    
    // Draw OK button on right side (no border, just text)
    int okX = 225;
    int okY = KEYPAD_START_Y;
    bool okSelected = (selectedKey == 11);
    if (okSelected) {
        canvas.setTextColor(COLOR_ACCENT);
    } else {
        canvas.setTextColor(COLOR_DIM);
    }
    canvas.setTextDatum(middle_center);
    // Draw OK vertically
    canvas.drawString("O", okX, okY + SIDE_BTN_HEIGHT/2 - 8);
    canvas.drawString("K", okX, okY + SIDE_BTN_HEIGHT/2 + 8);
    
    // Draw number keypad (3x3 grid for 1-9, then 0 centered at bottom)
    for (int i = 0; i < 10; i++) {
        int row, col, x, y;
        if (i < 9) {
            // Keys 1-9 in 3x3 grid
            row = i / 3;
            col = i % 3;
            x = KEYPAD_START_X + col * KEY_WIDTH;
            y = KEYPAD_START_Y + row * KEY_HEIGHT;
        } else {
            // Key 0 centered at bottom
            row = 3;
            col = 1;  // Center column
            x = KEYPAD_START_X + col * KEY_WIDTH;
            y = KEYPAD_START_Y + row * KEY_HEIGHT;
        }
        
        bool isSelected = (i == selectedKey);
        
        // Draw key background
        if (isSelected) {
            canvas.fillRoundRect(x + 2, y + 2, KEY_WIDTH - 4, KEY_HEIGHT - 4, 8, COLOR_ORANGE);
            canvas.setTextColor(COLOR_TEXT);
        } else {
            canvas.drawRoundRect(x + 2, y + 2, KEY_WIDTH - 4, KEY_HEIGHT - 4, 8, COLOR_DIM);
            canvas.setTextColor(COLOR_TEXT);
        }
        
        // Draw key label
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        char keyLabel[2] = {keypadChars[i], '\0'};
        canvas.drawString(keyLabel, x + KEY_WIDTH/2, y + KEY_HEIGHT/2);
        canvas.setTextSize(1);
    }
}

void drawSensorsScreen() {
    canvas.fillScreen(COLOR_BG);
    
    drawStatusBar();
    
    // Title
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(COLOR_ACCENT);
    canvas.drawString("SENSORS", CENTER_X, 48);
    
    // Draw sensor list in 2 columns
    int startY = 65;
    int lineHeight = 15;
    int colWidth = 115;  // Width for each column
    int col1X = 15;      // Left column X position
    int col2X = 125;     // Right column X position
    int rowsPerCol = (HA_SENSOR_COUNT + 1) / 2;  // Ceiling division
    
    canvas.setTextDatum(middle_left);
    
    for (int i = 0; i < HA_SENSOR_COUNT; i++) {
        int col = i / rowsPerCol;  // 0 = left column, 1 = right column
        int row = i % rowsPerCol;
        int x = (col == 0) ? col1X : col2X;
        int y = startY + row * lineHeight;
        
        // Status indicator
        if (haSensorOpen[i]) {
            canvas.fillCircle(x + 5, y, 4, COLOR_ERROR);
            canvas.setTextColor(COLOR_ERROR);
        } else {
            canvas.fillCircle(x + 5, y, 4, COLOR_OK);
            canvas.setTextColor(COLOR_OK);
        }
        
        // Sensor name (shortened to fit column)
        canvas.drawString(haSensorNames[i], x + 14, y);
    }
    
    // Hint
    canvas.setTextColor(COLOR_DIM);
    canvas.setTextDatum(middle_center);
    canvas.drawString("Press/Touch: Back", CENTER_X, 220);
}

void drawSettingsScreen() {
    canvas.fillScreen(COLOR_BG);
    
    drawStatusBar();
    
    // Title
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(COLOR_ORANGE);
    canvas.drawString("SETTINGS", CENTER_X, 48);
    
    // Get current RTC time
    auto dt = M5Dial.Rtc.getDateTime();
    
    // Field names and values
    const char* fieldNames[] = {"Hour", "Min", "Day", "Month", "Year"};
    int fieldValues[] = {
        dt.time.hours,
        dt.time.minutes,
        dt.date.date,
        dt.date.month,
        dt.date.year
    };
    
    int startY = 62;
    int lineHeight = 22;  // Reduced from 28 to fit all rows
    
    for (int i = 0; i < SET_FIELD_COUNT; i++) {
        int y = startY + i * lineHeight;
        bool isSelected = (i == settingsField);
        
        // Highlight selected field
        if (isSelected) {
            if (settingsEditing) {
                canvas.fillRoundRect(40, y - 8, 160, 18, 4, 0x03E0);  // Green when editing
            } else {
                canvas.fillRoundRect(40, y - 8, 160, 18, 4, COLOR_ORANGE);  // Orange when selected
            }
            canvas.setTextColor(COLOR_TEXT);
        } else {
            canvas.setTextColor(COLOR_DIM);
        }
        
        // Field name
        canvas.setTextDatum(middle_left);
        canvas.drawString(fieldNames[i], 50, y);
        
        // Field value
        canvas.setTextDatum(middle_right);
        char valStr[8];
        if (i == SET_YEAR) {
            snprintf(valStr, sizeof(valStr), "%04d", fieldValues[i]);
        } else {
            snprintf(valStr, sizeof(valStr), "%02d", fieldValues[i]);
        }
        canvas.drawString(valStr, 190, y);
    }
    
    // Hint - positioned to fit on screen
    canvas.setTextColor(COLOR_DIM);
    canvas.setTextDatum(middle_center);
    if (settingsEditing) {
        canvas.drawString("Turn: Adjust | Press: Next", CENTER_X, 185);
    } else {
        canvas.drawString("Turn: Select | Press: Edit", CENTER_X, 185);
    }
    canvas.drawString("Long Press: Back", CENTER_X, 200);
}

// Set RTC from compile time (__DATE__ and __TIME__ macros)
// Only sets if RTC year is before 2024 (indicates unset or reset RTC)
void setRtcFromCompileTime() {
    // Check current RTC time
    auto dt = M5Dial.Rtc.getDateTime();
    
    // If year is reasonable (>= 2024), assume RTC is already set
    if (dt.date.year >= 2024) {
        Serial.printf("RTC already set: %04d-%02d-%02d %02d:%02d:%02d\n",
                      dt.date.year, dt.date.month, dt.date.date,
                      dt.time.hours, dt.time.minutes, dt.time.seconds);
        return;
    }
    
    // Parse __DATE__ which is "Mmm DD YYYY" format (e.g., "Dec 27 2025")
    const char* dateStr = __DATE__;
    const char* timeStr = __TIME__;  // "HH:MM:SS" format
    
    // Month names
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    // Parse month
    int month = 1;
    for (int i = 0; i < 12; i++) {
        if (strncmp(dateStr, months[i], 3) == 0) {
            month = i + 1;
            break;
        }
    }
    
    // Parse day and year from __DATE__
    int day = atoi(dateStr + 4);
    int year = atoi(dateStr + 7);
    
    // Parse time from __TIME__
    int hour = atoi(timeStr);
    int minute = atoi(timeStr + 3);
    int second = atoi(timeStr + 6);
    
    // Set RTC
    m5::rtc_datetime_t newDt;
    newDt.date.year = year;
    newDt.date.month = month;
    newDt.date.date = day;
    newDt.time.hours = hour;
    newDt.time.minutes = minute;
    newDt.time.seconds = second;
    
    M5Dial.Rtc.setDateTime(newDt);
    
    Serial.printf("RTC set from compile time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  year, month, day, hour, minute, second);
}

// Sync RTC from NTP server (called when WiFi is connected)
void syncRtcFromNtp() {
    time_t now;
    time(&now);
    
    // Check if time is valid (after year 2020)
    if (now < 1577836800) {  // Jan 1, 2020 in Unix time
        // NTP not yet available, will retry next loop
        return;
    }
    
    // Add UTC+1 offset for CET (Italy winter time)
    now += 3600;  // Add 1 hour in seconds
    
    struct tm* timeinfo = gmtime(&now);
    
    // NTP time obtained successfully - update RTC
    m5::rtc_datetime_t newDt;
    newDt.date.year = timeinfo->tm_year + 1900;
    newDt.date.month = timeinfo->tm_mon + 1;
    newDt.date.date = timeinfo->tm_mday;
    newDt.time.hours = timeinfo->tm_hour;
    newDt.time.minutes = timeinfo->tm_min;
    newDt.time.seconds = timeinfo->tm_sec;
    
    M5Dial.Rtc.setDateTime(newDt);
    ntpSynced = true;
    
    Serial.printf("RTC synced from NTP: %04d-%02d-%02d %02d:%02d:%02d\n",
                  newDt.date.year, newDt.date.month, newDt.date.date,
                  newDt.time.hours, newDt.time.minutes, newDt.time.seconds);
}

// Shuffle the keypad numbers randomly (Fisher-Yates shuffle)
void shuffleKeypad() {
    // Reset to default first
    for (int i = 0; i < 12; i++) {
        keypadChars[i] = keypadCharsDefault[i];
    }
    
    // Shuffle only the digits (indices 0-9), keep CLR and OK in place
    for (int i = 9; i > 0; i--) {
        int j = random(0, i + 1);
        // Swap keypadChars[i] and keypadChars[j]
        char temp = keypadChars[i];
        keypadChars[i] = keypadChars[j];
        keypadChars[j] = temp;
    }
    
    Serial.print("Keypad shuffled: ");
    for (int i = 0; i < 10; i++) {
        Serial.print(keypadChars[i]);
    }
    Serial.println();
}

// Play alarm buzzer tone (alternating high-low siren)
void playAlarmBuzzer() {
    unsigned long now = millis();
    
    // Toggle between two frequencies every 800ms for slower siren effect
    if (now - lastBuzzerToggle >= 800) {
        lastBuzzerToggle = now;
        buzzerState = !buzzerState;
        
        if (buzzerState) {
            M5Dial.Speaker.tone(800, 800);   // High tone, longer duration
        } else {
            M5Dial.Speaker.tone(500, 800);   // Low tone, longer duration
        }
    }
}

// Stop alarm buzzer
void stopAlarmBuzzer() {
    alarmBuzzerActive = false;
    M5Dial.Speaker.stop();
}
