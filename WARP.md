# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

This is an ESP32 Height Monitor project that creates a complete IoT system for real-time height measurements using an ultrasonic sensor (HC-SR04). The system consists of three main components:

1. **ESP32 Firmware** - Arduino C++ code that reads ultrasonic sensor data and sends it over WiFi
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
# Key files: esp32-firmware/height_sensor.ino

# Required libraries (install via Arduino Library Manager):
# - ArduinoJson (for JSON serialization)
# - WiFi (built-in ESP32 library)
# - HTTPClient (built-in ESP32 library)
```

## Architecture Overview

### System Architecture
```
ESP32 (Sensor) → WiFi → Node.js Backend → Web Dashboard
     ↓                        ↓               ↓
HC-SR04 Sensor          REST API Server   Real-time UI
```

### Data Flow
1. **ESP32** reads ultrasonic sensor every 1 second (configurable)
2. **ESP32** sends JSON data via HTTP POST to backend API (`/api/height`)
3. **Backend** stores readings in-memory (last 100 readings) and serves REST API
4. **Frontend** polls backend every 2 seconds for current data and displays real-time updates

### Key Components

#### Backend API Server (`backend/server.js`)
- **Express.js** server with CORS enabled
- **In-memory storage** for recent readings (MAX_READINGS = 100)
- **Static file serving** for frontend assets
- **RESTful endpoints** for CRUD operations on height data

#### ESP32 Firmware (`esp32-firmware/height_sensor.ino`)
- **WiFi connectivity** with automatic connection handling
- **Ultrasonic sensor interface** using HC-SR04 (Trigger: GPIO 5, Echo: GPIO 18)
- **HTTP client** for JSON data transmission
- **Error handling** for invalid sensor readings (2-400cm range)

#### Frontend Dashboard (`frontend/`)
- **HeightMonitor class** manages all UI interactions and API calls
- **Real-time updates** with auto-refresh every 2 seconds
- **Statistics calculation** (min, max, average) from recent readings
- **Mobile-responsive design** with CSS Grid and Flexbox

### Configuration Points

#### ESP32 Configuration (height_sensor.ino)
```cpp
// WiFi credentials - MUST be updated before upload
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server URL - Update with actual server IP
const char* serverURL = "http://192.168.1.100:3000/api/height";

// Hardware pins (GPIO numbers)
#define TRIG_PIN 5    // Ultrasonic trigger pin
#define ECHO_PIN 18   // Ultrasonic echo pin

// Measurement interval
const int MEASUREMENT_INTERVAL = 1000; // milliseconds
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
ESP32 Pin    →    HC-SR04 Pin
GPIO 5       →    Trig
GPIO 18      →    Echo
3.3V         →    VCC
GND          →    GND
```

## Development Workflow

### Setting Up Development Environment
1. **Backend**: Run `cd backend && npm install && npm run dev`
2. **ESP32**: Configure WiFi credentials and server IP in `.ino` file
3. **ESP32**: Upload firmware using Arduino IDE (ensure libraries are installed)
4. **Testing**: Access web dashboard at `http://localhost:3000`

### Common Development Tasks

#### Testing ESP32 Communication
```bash
# Monitor ESP32 serial output to debug connection issues
# Check WiFi connection, HTTP responses, and sensor readings

# Test API endpoints manually before ESP32 testing
curl http://localhost:3000/api/health
```

#### Modifying Sensor Reading Frequency
- **ESP32**: Change `MEASUREMENT_INTERVAL` in `.ino` file
- **Frontend**: Modify `refreshIntervalMs` in `script.js`

#### Adding New API Endpoints
1. Add route handlers in `backend/server.js`
2. Update frontend API calls in `frontend/script.js`
3. Test with curl commands

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

- `backend/server.js` - Main API server logic and route definitions
- `esp32-firmware/height_sensor.ino` - Complete ESP32 firmware implementation
- `frontend/script.js` - Main frontend application class with all UI logic
- `backend/package.json` - Node.js dependencies and npm scripts