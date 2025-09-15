# ESP32 Height Monitor

A real-time web application that interfaces with an ESP32 microcontroller and ultrasonic sensor to measure and display height readings. The system consists of ESP32 firmware, a Node.js backend API server, and a responsive web frontend.

![ESP32 Height Monitor](https://img.shields.io/badge/ESP32-Height%20Monitor-blue)
![Node.js](https://img.shields.io/badge/Node.js-Backend-green)
![JavaScript](https://img.shields.io/badge/JavaScript-Frontend-yellow)

## 📋 Features

- **Real-time height measurements** using ultrasonic sensor (HC-SR04)
- **WiFi connectivity** for wireless data transmission
- **Web-based dashboard** with live data visualization
- **Automatic data refresh** every 2 seconds
- **Statistics tracking** (min, max, average heights)
- **Historical data storage** (in-memory)
- **Mobile-responsive design**
- **REST API endpoints** for data access

## 🔧 Hardware Requirements

- ESP32 development board
- HC-SR04 Ultrasonic sensor
- Jumper wires
- Breadboard (optional)
- Power supply (USB or external)

### Wiring Diagram

```
ESP32          HC-SR04
GPIO 5    -->  Trig
GPIO 18   -->  Echo
3.3V      -->  VCC
GND       -->  GND
```

## 🚀 Quick Start

### 1. Backend Setup

Navigate to the backend directory and install dependencies:

```bash
cd backend
npm install
```

Start the server:

```bash
npm start
```

The server will be available at `http://localhost:3000`

### 2. ESP32 Setup

1. Open `esp32-firmware/height_sensor.ino` in Arduino IDE
2. Install required libraries:
   - WiFi (built-in)
   - HTTPClient (built-in)
   - ArduinoJson
3. Update WiFi credentials and server IP:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* serverURL = "http://192.168.1.100:3000/api/height";
   ```
4. Upload the sketch to your ESP32

### 3. Frontend Access

Open your web browser and navigate to:
```
http://localhost:3000
```

## 📁 Project Structure

```
esp32-height-monitor/
├── esp32-firmware/
│   └── height_sensor.ino      # ESP32 Arduino sketch
├── backend/
│   ├── server.js              # Express.js API server
│   └── package.json           # Node.js dependencies
├── frontend/
│   ├── index.html             # Main HTML page
│   ├── style.css              # Styling
│   └── script.js              # Frontend JavaScript
├── docs/                      # Additional documentation
└── README.md                  # This file
```

## 🌐 API Endpoints

### Health Check
- **GET** `/api/health` - Server health status

### Height Data
- **POST** `/api/height` - Submit height reading (ESP32 → Server)
- **GET** `/api/height` - Get recent readings (limit parameter supported)
- **GET** `/api/height/current` - Get latest height reading
- **DELETE** `/api/height` - Clear all readings

## 📊 Web Interface

The web dashboard provides:

- **Large current reading display** with real-time updates
- **Connection status indicator** showing ESP32 connectivity
- **Statistics panel** displaying min, max, average, and total readings
- **Control buttons** for refresh, clear history, and auto-refresh toggle
- **Recent readings table** showing the last 20 measurements
- **Responsive design** that works on desktop and mobile devices

## ⚙️ Configuration

### ESP32 Configuration

In `esp32-firmware/height_sensor.ino`:

- **WiFi credentials**: Update `ssid` and `password`
- **Server URL**: Update `serverURL` with your server's IP address
- **Sensor pins**: Modify `TRIG_PIN` and `ECHO_PIN` if using different GPIOs
- **Measurement interval**: Adjust `MEASUREMENT_INTERVAL` (milliseconds)

### Backend Configuration

In `backend/server.js`:

- **Port**: Change `PORT` environment variable (default: 3000)
- **Max readings**: Modify `MAX_READINGS` for data retention
- **CORS settings**: Adjust if needed for production deployment

### Frontend Configuration

In `frontend/script.js`:

- **Auto-refresh interval**: Modify `refreshIntervalMs` (default: 2000ms)
- **API base URL**: Update if deploying to different domain

## 🛠️ Development

### Running in Development Mode

Backend with auto-reload:
```bash
cd backend
npm run dev
```

### Testing the API

Test height data submission:
```bash
curl -X POST http://localhost:3000/api/height \
  -H "Content-Type: application/json" \
  -d '{"height": 25.5, "sensor_id": "ESP32_001"}'
```

Get current reading:
```bash
curl http://localhost:3000/api/height/current
```

## 🔍 Troubleshooting

### ESP32 Won't Connect to WiFi
- Verify WiFi credentials in the code
- Check WiFi network compatibility (2.4GHz required)
- Monitor Serial output for connection status

### Web Interface Shows "Disconnected"
- Ensure backend server is running
- Check ESP32 server URL configuration
- Verify network connectivity between ESP32 and server

### No Height Readings Displayed
- Check ultrasonic sensor wiring
- Verify sensor is within measurement range (2-400cm)
- Monitor ESP32 Serial output for sensor readings

## 📈 Future Enhancements

- [ ] Database integration for persistent storage
- [ ] Data visualization charts and graphs  
- [ ] Multiple sensor support
- [ ] Email/SMS alerts for threshold values
- [ ] Data export functionality
- [ ] User authentication and authorization
- [ ] WebSocket integration for real-time updates
- [ ] Mobile app development

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆘 Support

For support and questions:

- Create an issue on GitHub
- Check the troubleshooting section
- Review the API documentation

## 🙏 Acknowledgments

- ESP32 community for excellent documentation
- Node.js and Express.js teams
- Contributors to the ArduinoJson library