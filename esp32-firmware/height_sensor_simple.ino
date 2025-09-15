#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- การตั้งค่าสำหรับหน้าจอ OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- การตั้งค่าสำหรับเซ็นเซอร์ ---
#define TRIG_PIN 5
#define ECHO_PIN 18

// --- การตั้งค่าปุ่มควบคุม ---
#define POWER_BUTTON 19  // ปุ่ม power สำหรับควบคุม

// --- การตั้งค่า WiFi ---
const char* ssid = "drwsWIFI_2.4GHz";  // เชื่อมต่ออัตโนมัติ
const char* password = "";              // ไม่มีรหัสผ่าน (open network)
const char* serverURL = "http://192.168.1.100:3000/api/height";

// --- การตั้งค่าการวัด ---
const int MEASUREMENT_INTERVAL = 1000;   // มิลลิวินาที
const float SOUND_SPEED = 0.034;         // ซม./ไมโครวินาที
const int MAX_READINGS = 20;             // เก็บข้อมูล 20 ครั้งล่าสุด
const int DISPLAY_CYCLE_TIME = 5000;     // เปลี่ยนหน้าจอทุก 5 วินาที
const int BUTTON_HOLD_TIME = 2000;       // กดค้างไว้ 2 วินาที

// --- โครงสร้างข้อมูลการวัด ---
struct Reading {
  float height;
  unsigned long timestamp;
  bool valid;
};

// --- ตัวแปรเก็บข้อมูล ---
Reading readings[MAX_READINGS];
int currentIndex = 0;
int totalReadings = 0;

// --- สถิติ ---
float minHeight = 999999;
float maxHeight = 0;
float totalHeight = 0;
int validReadings = 0;

// --- โหมดการแสดงผล ---
enum DisplayMode {
  MODE_CURRENT = 0,
  MODE_STATS = 1,
  MODE_HISTORY = 2,
  MODE_SETTINGS = 3
};
DisplayMode displayMode = MODE_CURRENT;

// --- สถานะระบบ ---
bool wifiEnabled = true;
bool wifiConnected = false;
bool autoDisplayCycle = true;
String deviceIP = "";
unsigned long lastMeasurement = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDisplayCycle = 0;
unsigned long bootTime = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);
  bootTime = millis();
  
  // เริ่มต้นหน้าจอ OLED
  if (!OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  OLED.display();
  delay(1000);
  OLED.clearDisplay();
  
  // เริ่มต้นเซ็นเซอร์
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // เริ่มต้นปุ่ม
  pinMode(POWER_BUTTON, INPUT_PULLUP);
  
  // เริ่มต้นข้อมูล
  initializeData();
  
  // แสดงหน้าจอเริ่มต้น
  showBootScreen();
  delay(2000);
  
  // เชื่อมต่อ WiFi อัตโนมัติ
  connectToWiFi();
  
  Serial.println("ESP32 Height Monitor - Simple Mode Ready");
  Serial.print("Device IP: ");
  Serial.println(deviceIP);
}

void loop() {
  unsigned long now = millis();
  
  // ตรวจสอบปุ่ม
  checkButton();
  
  // วัดความสูง
  if (now - lastMeasurement >= MEASUREMENT_INTERVAL) {
    performMeasurement();
    lastMeasurement = now;
  }
  
  // อัพเดทหน้าจอ
  if (now - lastDisplayUpdate >= 500) { // อัพเดททุก 500ms
    updateDisplay();
    lastDisplayUpdate = now;
  }
  
  // เปลี่ยนหน้าจอแสดงผลอัตโนมัติ (ถ้าเปิดใช้งาน)
  if (autoDisplayCycle && (now - lastDisplayCycle >= DISPLAY_CYCLE_TIME)) {
    displayMode = (DisplayMode)((displayMode + 1) % 4);
    lastDisplayCycle = now;
    Serial.print("Auto display mode changed to: ");
    Serial.println(displayMode);
  }
  
  // ตรวจสอบการรีเซ็ตอัตโนมัติทุก 1 ชั่วโมง
  checkAutoReset();
  
  // ตรวจสอบสถานะ WiFi
  if (wifiEnabled && (now % 30000 == 0)) {
    checkWiFiStatus();
  }
  
  delay(50);
}

void initializeData() {
  for (int i = 0; i < MAX_READINGS; i++) {
    readings[i].height = 0;
    readings[i].timestamp = 0;
    readings[i].valid = false;
  }
  resetStatistics();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
  
  showWiFiConnecting();
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // อัพเดทหน้าจอระหว่างเชื่อมต่อ
    if (attempts % 4 == 0) {
      showWiFiConnecting();
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    deviceIP = WiFi.localIP().toString();
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(deviceIP);
    
    // แสดงหน้าจอเชื่อมต่อสำเร็จ
    showWiFiConnected();
    delay(2000);
  } else {
    wifiConnected = false;
    deviceIP = "";
    Serial.println();
    Serial.println("WiFi connection failed");
    
    showWiFiError();
    delay(2000);
  }
}

void performMeasurement() {
  float height = measureHeight();
  
  if (height > 0) {
    // เก็บข้อมูล
    storeReading(height);
    updateStatistics(height);
    
    // แสดงใน Serial Monitor
    Serial.print("Height: ");
    Serial.print(height, 1);
    Serial.println(" cm");
    
    // ส่งข้อมูลผ่าน WiFi (ถ้าเชื่อมต่อ)
    if (wifiEnabled && wifiConnected) {
      sendToWebServer(height);
    }
  }
}

float measureHeight() {
  // เคลียร์ trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // ส่งสัญญาณ 10us pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // อ่านค่าจาก echo pin
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  
  if (duration == 0) {
    return -1; // timeout
  }
  
  // คำนวณระยะทาง
  float distance = duration * SOUND_SPEED / 2;
  
  // กรองค่าที่ไม่ถูกต้อง
  if (distance < 2 || distance > 400) {
    return -1;
  }
  
  return distance;
}

void storeReading(float height) {
  readings[currentIndex].height = height;
  readings[currentIndex].timestamp = millis();
  readings[currentIndex].valid = true;
  
  currentIndex = (currentIndex + 1) % MAX_READINGS;
  totalReadings++;
}

void updateStatistics(float height) {
  validReadings++;
  totalHeight += height;
  
  if (height < minHeight) minHeight = height;
  if (height > maxHeight) maxHeight = height;
}

void resetStatistics() {
  minHeight = 999999;
  maxHeight = 0;
  totalHeight = 0;
  validReadings = 0;
  totalReadings = 0;
  currentIndex = 0;
  
  // เคลียร์ข้อมูลทั้งหมด
  for (int i = 0; i < MAX_READINGS; i++) {
    readings[i].valid = false;
  }
  
  Serial.println("Statistics reset");
}

float getAverageHeight() {
  if (validReadings == 0) return 0;
  return totalHeight / validReadings;
}

void checkButton() {
  bool currentButtonState = (digitalRead(POWER_BUTTON) == LOW);
  
  if (currentButtonState && !buttonPressed) {
    // ปุ่มเริ่มถูกกด
    buttonPressed = true;
    buttonPressStart = millis();
  } else if (!currentButtonState && buttonPressed) {
    // ปุ่มถูกปล่อย
    unsigned long pressDuration = millis() - buttonPressStart;
    buttonPressed = false;
    
    if (pressDuration < BUTTON_HOLD_TIME) {
      // กดสั้น - เปลี่ยนโหมดหน้าจอ
      displayMode = (DisplayMode)((displayMode + 1) % 4);
      lastDisplayCycle = millis(); // รีเซ็ตตัวจับเวลาอัตโนมัติ
      Serial.print("Manual display mode changed to: ");
      Serial.println(displayMode);
    } else {
      // กดยาว - รีเซ็ตสถิติ
      resetStatistics();
      Serial.println("Manual statistics reset");
      
      // แสดงข้อความยืนยัน
      OLED.clearDisplay();
      OLED.setTextSize(2);
      OLED.setTextColor(SSD1306_WHITE);
      OLED.setCursor(20, 20);
      OLED.println("RESET");
      OLED.setCursor(10, 40);
      OLED.println("COMPLETE");
      OLED.display();
      delay(1500);
    }
  }
}

void checkAutoReset() {
  static unsigned long lastAutoReset = 0;
  const unsigned long AUTO_RESET_INTERVAL = 3600000; // 1 ชั่วโมง
  
  if (millis() - lastAutoReset >= AUTO_RESET_INTERVAL) {
    resetStatistics();
    lastAutoReset = millis();
    Serial.println("Auto statistics reset after 1 hour");
  }
}

void checkWiFiStatus() {
  if (wifiEnabled && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    deviceIP = "";
    Serial.println("WiFi disconnected, attempting reconnection...");
    connectToWiFi();
  }
}

void sendToWebServer(float height) {
  if (!wifiConnected) return;
  
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(1024);
  doc["height"] = height;
  doc["timestamp"] = millis();
  doc["sensor_id"] = "ESP32_SIMPLE";
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  int httpCode = http.POST(requestBody);
  
  if (httpCode > 0) {
    Serial.println("Data sent to web server");
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

void showBootScreen() {
  OLED.clearDisplay();
  OLED.setTextSize(2);
  OLED.setTextColor(SSD1306_WHITE);
  OLED.setCursor(0, 0);
  OLED.println("ESP32");
  OLED.println("HEIGHT");
  OLED.setTextSize(1);
  OLED.setCursor(0, 40);
  OLED.println("MONITOR");
  OLED.setCursor(0, 55);
  OLED.println("Simple Mode Ready");
  OLED.display();
}

void showWiFiConnecting() {
  OLED.clearDisplay();
  OLED.setTextSize(1);
  OLED.setTextColor(SSD1306_WHITE);
  OLED.setCursor(0, 10);
  OLED.println("Connecting to WiFi...");
  OLED.setCursor(0, 25);
  OLED.println(ssid);
  OLED.setCursor(0, 45);
  OLED.println("Please wait...");
  OLED.display();
}

void showWiFiConnected() {
  OLED.clearDisplay();
  OLED.setTextSize(1);
  OLED.setTextColor(SSD1306_WHITE);
  OLED.setCursor(0, 5);
  OLED.println("WiFi Connected!");
  OLED.setCursor(0, 20);
  OLED.println("Network:");
  OLED.setCursor(0, 30);
  OLED.println(ssid);
  OLED.setCursor(0, 45);
  OLED.println("IP Address:");
  OLED.setCursor(0, 55);
  OLED.println(deviceIP);
  OLED.display();
}

void showWiFiError() {
  OLED.clearDisplay();
  OLED.setTextSize(1);
  OLED.setTextColor(SSD1306_WHITE);
  OLED.setCursor(0, 15);
  OLED.println("WiFi Connection Failed");
  OLED.setCursor(0, 30);
  OLED.println("Running offline mode");
  OLED.setCursor(0, 45);
  OLED.println("Check network settings");
  OLED.display();
}

void updateDisplay() {
  OLED.clearDisplay();
  OLED.setTextColor(SSD1306_WHITE);
  
  switch (displayMode) {
    case MODE_CURRENT:
      showCurrentReading();
      break;
    case MODE_STATS:
      showStatistics();
      break;
    case MODE_HISTORY:
      showHistory();
      break;
    case MODE_SETTINGS:
      showSettings();
      break;
  }
  
  OLED.display();
}

void showCurrentReading() {
  // หัวข้อ
  OLED.setTextSize(1);
  OLED.setCursor(0, 0);
  OLED.println("CURRENT HEIGHT");
  
  // ค่าปัจจุบัน (ตัวใหญ่)
  OLED.setTextSize(2);
  OLED.setCursor(0, 15);
  
  if (validReadings > 0) {
    // หาค่าล่าสุดที่ถูกต้อง
    int lastValid = (currentIndex - 1 + MAX_READINGS) % MAX_READINGS;
    while (!readings[lastValid].valid && lastValid != currentIndex) {
      lastValid = (lastValid - 1 + MAX_READINGS) % MAX_READINGS;
    }
    
    if (readings[lastValid].valid) {
      OLED.print(readings[lastValid].height, 1);
    } else {
      OLED.print("--");
    }
  } else {
    OLED.print("--");
  }
  OLED.println(" cm");
  
  // สถานะ
  OLED.setTextSize(1);
  OLED.setCursor(0, 40);
  OLED.print("Readings: ");
  OLED.println(validReadings);
  
  OLED.setCursor(0, 50);
  OLED.print("WiFi: ");
  OLED.print(wifiConnected ? "ON" : "OFF");
  
  // แสดง IP (ถ้ามี)
  if (wifiConnected && deviceIP.length() > 0) {
    OLED.setCursor(60, 50);
    OLED.println(deviceIP);
  }
  
  // แสดงโหมด
  OLED.setCursor(100, 0);
  OLED.println("1/4");
}

void showStatistics() {
  OLED.setTextSize(1);
  OLED.setCursor(0, 0);
  OLED.println("STATISTICS");
  OLED.setCursor(100, 0);
  OLED.println("2/4");
  
  OLED.setCursor(0, 15);
  OLED.print("Min: ");
  OLED.print(minHeight == 999999 ? 0 : minHeight, 1);
  OLED.println(" cm");
  
  OLED.setCursor(0, 25);
  OLED.print("Max: ");
  OLED.print(maxHeight, 1);
  OLED.println(" cm");
  
  OLED.setCursor(0, 35);
  OLED.print("Avg: ");
  OLED.print(getAverageHeight(), 1);
  OLED.println(" cm");
  
  OLED.setCursor(0, 45);
  OLED.print("Total: ");
  OLED.println(validReadings);
  
  // แสดงเวลาทำงาน
  unsigned long uptime = (millis() - bootTime) / 1000;
  OLED.setCursor(0, 55);
  OLED.print("Uptime: ");
  OLED.print(uptime);
  OLED.println("s");
}

void showHistory() {
  OLED.setTextSize(1);
  OLED.setCursor(0, 0);
  OLED.println("RECENT READINGS");
  OLED.setCursor(100, 0);
  OLED.println("3/4");
  
  int y = 15;
  int count = 0;
  
  // แสดง 5 ค่าล่าสุด
  int idx = (currentIndex - 1 + MAX_READINGS) % MAX_READINGS;
  while (count < 5 && count < validReadings) {
    if (readings[idx].valid) {
      OLED.setCursor(0, y);
      OLED.print(readings[idx].height, 1);
      OLED.print(" cm ");
      
      // แสดงเวลาที่ผ่านมา
      unsigned long ago = (millis() - readings[idx].timestamp) / 1000;
      OLED.print(ago);
      OLED.println("s ago");
      
      y += 8;
      count++;
    }
    idx = (idx - 1 + MAX_READINGS) % MAX_READINGS;
  }
  
  if (count == 0) {
    OLED.setCursor(0, 30);
    OLED.println("No data available");
  }
}

void showSettings() {
  OLED.setTextSize(1);
  OLED.setCursor(0, 0);
  OLED.println("SETTINGS");
  OLED.setCursor(100, 0);
  OLED.println("4/4");
  
  OLED.setCursor(0, 15);
  OLED.print("WiFi: ");
  OLED.println(wifiConnected ? "CONNECTED" : "DISCONNECTED");
  
  OLED.setCursor(0, 25);
  OLED.print("Network: ");
  OLED.println(ssid);
  
  OLED.setCursor(0, 35);
  OLED.print("IP: ");
  OLED.println(wifiConnected ? deviceIP : "N/A");
  
  OLED.setCursor(0, 45);
  OLED.println("CONTROLS:");
  OLED.setCursor(0, 55);
  OLED.println("SHORT:MODE LONG:RESET");
}