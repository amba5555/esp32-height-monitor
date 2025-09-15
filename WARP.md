# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

This is an ESP32 Height Monitor project that creates a complete IoT system for real-time height measurements using an ultrasonic sensor (HC-SR04). The system is designed with a **standalone-first approach** where the ESP32 works independently with an OLED display, and web connectivity is secondary.

The system consists of three main components:

1. **ESP32 Firmware** - Arduino C++ code with OLED display that works independently, with optional WiFi connectivity
2. **Node.js Backend** - Express.js API server that receives sensor data and serves the web interface
3. **Frontend Web Interface** - Vanilla JavaScript/HTML/CSS dashboard for real-time monitoring

## Development Commands

### Backend Development
```bash
# Navigate to backend directory
cd backend

# Install dependencies
npm install

# Start production server
npm start

# Start development server with auto-reload (recommended for development)
npm run dev

# The server runs on http://localhost:3000 by default
```

### API Testing
```bash
# Test health endpoint
curl http://localhost:3000/api/health

# Get current height reading
curl http://localhost:3000/api/height/current

# Get recent readings (with limit)
curl http://localhost:3000/api/height?limit=10

# Simulate ESP32 data submission
curl -X POST http://localhost:3000/api/height \
  -H "Content-Type: application/json" \
  -d '{"height": 25.5, "sensor_id": "ESP32_001"}'

# Clear all readings
curl -X DELETE http://localhost:3000/api/height
```

### ESP32 Firmware Development
```bash
# The ESP32 firmware must be uploaded using Arduino IDE
# Key files: 
#   - esp32-firmware/height_sensor.ino (original web-dependent version)
#   - esp32-firmware/height_sensor_simple.ino (standalone-first version)

# Required libraries (install via Arduino Library Manager):
# - Adafruit GFX Library
# - Adafruit SSD1306 (for OLED display)
# - ArduinoJson (for JSON serialization)
# - WiFi (built-in ESP32 library)
# - HTTPClient (built-in ESP32 library)
```

## Architecture Overview

### System Architecture (Standalone-First Design)
```
ESP32 (Standalone) ←→ OLED Display (Primary Interface)
     ↓                        
HC-SR04 Sensor               
     ↓ (Optional)             
WiFi → Node.js Backend → Web Dashboard (Secondary)
            ↓                    ↓
    REST API Server      Real-time Web UI
```

### Data Flow
1. **ESP32** reads ultrasonic sensor every 1 second (configurable)
2. **ESP32** stores readings locally and displays on OLED (primary operation)
3. **ESP32** calculates statistics (min/max/average) locally and shows on display
4. **ESP32** optionally sends JSON data via HTTP POST to backend API (`/api/height`) if WiFi is connected
5. **Backend** stores readings in-memory (last 100 readings) and serves REST API
6. **Frontend** polls backend every 2 seconds for current data and displays real-time updates

### Key Components

#### Backend API Server (`backend/server.js`)
- **Express.js** server with CORS enabled
- **In-memory storage** for recent readings (MAX_READINGS = 100)
- **Static file serving** for frontend assets
- **RESTful endpoints** for CRUD operations on height data

#### ESP32 Firmware (`esp32-firmware/height_sensor_simple.ino`)
- **Standalone operation** with OLED display as primary interface
- **Local data storage** for 20 recent readings with statistics calculation
- **Auto-cycling display** with 4 modes: Current, Statistics, History, Settings
- **Single button control** with short press (mode change) and long press (reset)
- **WiFi connectivity** as optional secondary feature
- **Ultrasonic sensor interface** using HC-SR04 (Trigger: GPIO 5, Echo: GPIO 18)
- **HTTP client** for JSON data transmission when WiFi is available
- **Error handling** for invalid sensor readings (2-400cm range)

#### Frontend Dashboard (`frontend/`)
- **HeightMonitor class** manages all UI interactions and API calls
- **Real-time updates** with auto-refresh every 2 seconds
- **Statistics calculation** (min, max, average) from recent readings
- **Mobile-responsive design** with CSS Grid and Flexbox

### Configuration Points

#### ESP32 Configuration (height_sensor_simple.ino)
```cpp
// WiFi credentials - Automatically connects to open network
const char* ssid = "drwsWIFI_2.4GHz";  // Open network, no password
const char* password = "";              // Empty for open network

// Server URL - Update with actual server IP
const char* serverURL = "http://192.168.1.100:3000/api/height";

// Hardware pins (GPIO numbers)
#define TRIG_PIN 5       // Ultrasonic trigger pin
#define ECHO_PIN 18      // Ultrasonic echo pin
#define POWER_BUTTON 19  // Single control button

// Display and timing settings
const int MEASUREMENT_INTERVAL = 1000;   // milliseconds
const int DISPLAY_CYCLE_TIME = 5000;     // Auto-cycle display every 5 seconds
const int BUTTON_HOLD_TIME = 2000;       // Long press threshold
```

#### Backend Configuration (server.js)
```javascript
const PORT = process.env.PORT || 3000;        // Server port
const MAX_READINGS = 100;                     // In-memory storage limit
```

#### Frontend Configuration (script.js)
```javascript
this.refreshIntervalMs = 2000;               // Auto-refresh interval
this.apiBase = '/api';                       // API base path
```

### Hardware Setup
```
// Ultrasonic Sensor
ESP32 Pin    →    HC-SR04 Pin
GPIO 5       →    Trig
GPIO 18      →    Echo
5V           →    VCC (or 3.3V)
GND          →    GND

// OLED Display (I2C)
ESP32 Pin    →    SSD1306 OLED
GPIO 21      →    SDA
GPIO 22      →    SCL
3.3V         →    VCC
GND          →    GND

// Control Button
ESP32 Pin    →    Push Button
GPIO 19      →    One side of button
GND          →    Other side of button
```

## Development Workflow

### Setting Up Development Environment
1. **ESP32 Standalone**: Upload `height_sensor_simple.ino` using Arduino IDE (works without WiFi)
2. **Backend** (Optional): Run `cd backend && npm install && npm run dev`
3. **ESP32**: WiFi automatically connects to "drwsWIFI_2.4GHz" if available
4. **Testing**: Device works immediately with OLED display, web dashboard at `http://localhost:3000` if connected

### Common Development Tasks

#### Testing ESP32 Standalone Operation
```bash
# Monitor ESP32 serial output via Arduino IDE Serial Monitor
# Check sensor readings, OLED display updates, and button responses
# WiFi connection status and IP address will be shown

# Test API endpoints (if WiFi connected)
curl http://localhost:3000/api/health
```

#### Button Controls
- **Short Press** (< 2 seconds): Change display mode (Current → Stats → History → Settings)
- **Long Press** (≥ 2 seconds): Reset all statistics and stored readings
- **Auto-Cycle**: Display automatically cycles every 5 seconds if no manual interaction

#### Display Modes
1. **Current Reading**: Large height display with WiFi status and IP
2. **Statistics**: Min/Max/Average heights and total readings
3. **History**: Last 5 readings with timestamps
4. **Settings**: WiFi status, network info, and control instructions

#### Modifying Settings
- **ESP32**: Change `MEASUREMENT_INTERVAL`, `DISPLAY_CYCLE_TIME` in `.ino` file
- **Frontend**: Modify `refreshIntervalMs` in `script.js`
- **WiFi**: Update `ssid` and `password` in ESP32 code

### Troubleshooting

#### ESP32 Issues
- **WiFi connection failures**: Verify network credentials and 2.4GHz compatibility
- **HTTP errors**: Check server URL and network connectivity
- **Invalid sensor readings**: Verify HC-SR04 wiring and measurement range (2-400cm)

#### Backend Issues
- **Port conflicts**: Change PORT environment variable
- **CORS errors**: Verify CORS configuration for cross-origin requests

#### Frontend Issues
- **API connection failures**: Check backend server status and network connectivity
- **Real-time updates not working**: Verify auto-refresh is enabled and API responses

## File Structure Importance

- `esp32-firmware/height_sensor_simple.ino` - **Primary ESP32 firmware** (standalone-first design)
- `esp32-firmware/height_sensor.ino` - Original web-dependent ESP32 firmware
- `backend/server.js` - Main API server logic and route definitions
- `frontend/script.js` - Main frontend application class with all UI logic
- `backend/package.json` - Node.js dependencies and npm scripts

## Development Philosophy

This project follows a **standalone-first approach**:
1. **Primary Operation**: ESP32 works independently with OLED display
2. **Secondary Feature**: Web connectivity enhances but doesn't replace local functionality
3. **Resilient Design**: System continues operating even if WiFi/backend fails
4. **User Experience**: Immediate feedback via OLED display, web dashboard as additional interface
