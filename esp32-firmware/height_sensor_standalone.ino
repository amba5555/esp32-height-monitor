#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
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
#define BUTTON_MODE 19      // ปุ่มสำหรับเปลี่ยนหน้าจอ
#define BUTTON_RESET 21     // ปุ่มสำหรับรีเซ็ตข้อมูล
#define BUTTON_WIFI 22      // ปุ่มสำหรับเปิด/ปิด WiFi

// --- การตั้งค่า WiFi (ใช้เมื่อเปิดใช้งาน) ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverURL = "http://192.168.1.100:3000/api/height";

// --- การตั้งค่าการวัด ---
const int MEASUREMENT_INTERVAL = 1000; // มิลลิวินาที
const float SOUND_SPEED = 0.034; // ซม./ไมโครวินาที
const int MAX_READINGS = 20; // เก็บข้อมูล 20 ครั้งล่าสุด

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
bool wifiEnabled = false;
bool wifiConnected = false;
bool bleEnabled = true;
unsigned long lastMeasurement = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long bootTime = 0;

// --- BLE Variables ---
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

// --- BLE Callbacks ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

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
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_WIFI, INPUT_PULLUP);
  
  // เริ่มต้น BLE
  initializeBLE();
  
  // เริ่มต้นข้อมูล
  initializeData();
  
  // แสดงหน้าจอเริ่มต้น
  showBootScreen();
  delay(2000);
  
  Serial.println("ESP32 Height Monitor - Standalone Mode Ready");
}

void loop() {
  unsigned long now = millis();
  
  // ตรวจสอบปุ่ม
  checkButtons();
  
  // วัดความสูง
  if (now - lastMeasurement >= MEASUREMENT_INTERVAL) {
    performMeasurement();
    lastMeasurement = now;
  }
  
  // อัพเดทหน้าจอ
  if (now - lastDisplayUpdate >= 300) { // อัพเดททุก 300ms
    updateDisplay();
    lastDisplayUpdate = now;
  }
  
  // ตรวจสอบสถานะ WiFi (ถ้าเปิดใช้งาน)
  if (wifiEnabled && (now % 30000 == 0)) {
    checkWiFiStatus();
  }
  
  delay(50); // หน่วงเวลาเล็กน้อยเพื่อให้ระบบทำงานได้อย่างราบรื่น
}

void initializeBLE() {
  BLEDevice::init("ESP32-HeightMonitor");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  pServer->startAdvertising();
  
  Serial.println("BLE service started, waiting for connections...");
}

void initializeData() {
  for (int i = 0; i < MAX_READINGS; i++) {
    readings[i].height = 0;
    readings[i].timestamp = 0;
    readings[i].valid = false;
  }
  resetStatistics();
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
    
    // ส่งข้อมูลผ่าน BLE
    if (deviceConnected && bleEnabled) {
      String data = String(height, 1);
      pCharacteristic->setValue(data.c_str());
      pCharacteristic->notify();
    }
    
    // ส่งข้อมูลผ่าน WiFi (ถ้าเปิดใช้งานและเชื่อมต่อ)
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

void checkButtons() {
  static bool modePressed = false;
  static bool resetPressed = false;
  static bool wifiPressed = false;
  static unsigned long lastButtonPress = 0;
  
  unsigned long now = millis();
  if (now - lastButtonPress < 200) return; // debounce
  
  // ปุ่มเปลี่ยนโหมด
  if (digitalRead(BUTTON_MODE) == LOW && !modePressed) {
    modePressed = true;
    displayMode = (DisplayMode)((displayMode + 1) % 4);
    lastButtonPress = now;
    Serial.print("Display mode changed to: ");
    Serial.println(displayMode);
  } else if (digitalRead(BUTTON_MODE) == HIGH) {
    modePressed = false;
  }
  
  // ปุ่มรีเซ็ต
  if (digitalRead(BUTTON_RESET) == LOW && !resetPressed) {
    resetPressed = true;
    resetStatistics();
    lastButtonPress = now;
  } else if (digitalRead(BUTTON_RESET) == HIGH) {
    resetPressed = false;
  }
  
  // ปุ่ม WiFi
  if (digitalRead(BUTTON_WIFI) == LOW && !wifiPressed) {
    wifiPressed = true;
    toggleWiFi();
    lastButtonPress = now;
  } else if (digitalRead(BUTTON_WIFI) == HIGH) {
    wifiPressed = false;
  }
}

void toggleWiFi() {
  wifiEnabled = !wifiEnabled;
  
  if (wifiEnabled) {
    Serial.println("Enabling WiFi...");
    connectToWiFi();
  } else {
    Serial.println("Disabling WiFi...");
    WiFi.disconnect();
    wifiConnected = false;
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("WiFi failed to connect");
  }
}

void checkWiFiStatus() {
  if (wifiEnabled && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    Serial.println("WiFi disconnected, reconnecting...");
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
  doc["sensor_id"] = "ESP32_STANDALONE";
  
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
  OLED.println("Standalone Ready");
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
  OLED.print(wifiEnabled ? (wifiConnected ? "ON" : "OFF") : "DIS");
  OLED.print(" BLE: ");
  OLED.println(deviceConnected ? "CON" : "ADV");
  
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
  OLED.println(wifiEnabled ? "ENABLED" : "DISABLED");
  
  OLED.setCursor(0, 25);
  OLED.print("BLE: ");
  OLED.println(bleEnabled ? "ENABLED" : "DISABLED");
  
  OLED.setCursor(0, 35);
  OLED.print("Interval: ");
  OLED.print(MEASUREMENT_INTERVAL);
  OLED.println("ms");
  
  OLED.setCursor(0, 45);
  OLED.println("CONTROLS:");
  OLED.setCursor(0, 55);
  OLED.println("19:MODE 21:RST 22:WIFI");
}