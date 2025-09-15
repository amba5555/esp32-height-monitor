#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server configuration
const char* serverURL = "http://192.168.1.100:3000/api/height"; // Update with your server IP

// Ultrasonic sensor pins
#define TRIG_PIN 5
#define ECHO_PIN 18

// Measurement settings
const int MEASUREMENT_INTERVAL = 1000; // milliseconds
const float SOUND_SPEED = 0.034; // cm/microsecond

void setup() {
  Serial.begin(115200);
  
  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  float height = measureHeight();
  
  if (height > 0) {
    Serial.print("Height: ");
    Serial.print(height);
    Serial.println(" cm");
    
    sendHeightData(height);
  }
  
  delay(MEASUREMENT_INTERVAL);
}

float measureHeight() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10us pulse to trigger pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance (height)
  float distance = duration * SOUND_SPEED / 2;
  
  // Filter out invalid readings
  if (distance < 2 || distance > 400) {
    return -1; // Invalid reading
  }
  
  return distance;
}

void sendHeightData(float height) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload
    DynamicJsonDocument doc(1024);
    doc["height"] = height;
    doc["timestamp"] = millis();
    doc["sensor_id"] = "ESP32_001";
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    int httpResponseCode = http.POST(requestBody);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Data sent successfully");
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error sending data. HTTP response code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}