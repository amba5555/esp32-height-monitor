const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../frontend')));

// Store recent height readings (in-memory for simplicity)
let heightReadings = [];
const MAX_READINGS = 100; // Keep last 100 readings

// API Routes

// Receive height data from ESP32
app.post('/api/height', (req, res) => {
    const { height, timestamp, sensor_id } = req.body;
    
    if (height === undefined) {
        return res.status(400).json({ error: 'Height value is required' });
    }
    
    const reading = {
        height: parseFloat(height),
        timestamp: timestamp || Date.now(),
        sensor_id: sensor_id || 'unknown',
        received_at: new Date().toISOString()
    };
    
    // Add new reading
    heightReadings.push(reading);
    
    // Keep only the most recent readings
    if (heightReadings.length > MAX_READINGS) {
        heightReadings = heightReadings.slice(-MAX_READINGS);
    }
    
    console.log(`New height reading: ${height} cm from ${sensor_id}`);
    
    res.json({ 
        success: true, 
        message: 'Height data received',
        reading: reading
    });
});

// Get latest height readings
app.get('/api/height', (req, res) => {
    const limit = parseInt(req.query.limit) || 10;
    const recentReadings = heightReadings.slice(-limit);
    
    res.json({
        readings: recentReadings,
        total: heightReadings.length
    });
});

// Get current/latest height
app.get('/api/height/current', (req, res) => {
    if (heightReadings.length === 0) {
        return res.json({ 
            height: null, 
            message: 'No readings available' 
        });
    }
    
    const latest = heightReadings[heightReadings.length - 1];
    res.json(latest);
});

// Clear all readings (useful for testing)
app.delete('/api/height', (req, res) => {
    heightReadings = [];
    res.json({ 
        success: true, 
        message: 'All readings cleared' 
    });
});

// Health check endpoint
app.get('/api/health', (req, res) => {
    res.json({ 
        status: 'healthy',
        uptime: process.uptime(),
        readings_count: heightReadings.length,
        timestamp: new Date().toISOString()
    });
});

// Serve frontend
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, '../frontend/index.html'));
});

// Start server
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
    console.log(`Frontend available at: http://localhost:${PORT}`);
    console.log(`API endpoints:`);
    console.log(`  POST /api/height - Receive height data from ESP32`);
    console.log(`  GET /api/height - Get recent height readings`);
    console.log(`  GET /api/height/current - Get latest height reading`);
    console.log(`  GET /api/health - Health check`);
});

module.exports = app;