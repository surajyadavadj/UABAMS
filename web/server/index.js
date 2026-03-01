const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mqtt = require('mqtt');
const { Pool } = require('pg');
const cors = require('cors');

const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
    cors: {
        origin: "http://localhost:3000",
        methods: ["GET", "POST"]
    }
});

app.use(cors());
app.use(express.json());

// PostgreSQL connection
const pool = new Pool({
    user: 'your_user',
    host: 'localhost',
    database: 'railway_monitoring',
    password: 'your_password',
    port: 5432,
});

// MQTT connection for sensor data
const mqttClient = mqtt.connect('mqtt://localhost:1883');

// API Endpoints

// Get recent impact events
app.get('/api/impacts', async (req, res) => {
    try {
        const { rows } = await pool.query(
            'SELECT * FROM accelerometer_events ORDER BY timestamp DESC LIMIT 50'
        );
        res.json(rows);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Get monitoring data for graphs
app.get('/api/monitoring/:timeRange', async (req, res) => {
    const { timeRange } = req.params;
    let interval = '1 minute';
    
    if (timeRange === '1h') interval = '1 minute';
    else if (timeRange === '24h') interval = '5 minutes';
    else if (timeRange === '7d') interval = '1 hour';
    
    try {
        const { rows } = await pool.query(`
            SELECT time_bucket($1, timestamp) as bucket,
                   avg(x_axis) as avg_x,
                   avg(y_axis) as avg_y,
                   avg(z_axis) as avg_z
            FROM monitoring_data
            WHERE timestamp > NOW() - INTERVAL '1 day'
            GROUP BY bucket
            ORDER BY bucket DESC
        `, [interval]);
        res.json(rows);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// WebSocket for real-time updates
io.on('connection', (socket) => {
    console.log('Client connected');
    
    socket.on('disconnect', () => {
        console.log('Client disconnected');
    });
});

// MQTT message handling
mqttClient.on('connect', () => {
    mqttClient.subscribe('sensor/accelerometer/#');
    mqttClient.subscribe('sensor/gps/#');
});

mqttClient.on('message', async (topic, message) => {
    const data = JSON.parse(message.toString());
    
    // Store in database
    if (topic.includes('accelerometer')) {
        await pool.query(
            'INSERT INTO monitoring_data (x_axis, y_axis, z_axis, timestamp) VALUES ($1, $2, $3, $4)',
            [data.x, data.y, data.z, new Date(data.timestamp)]
        );
    }
    
    // Emit to all connected clients
    io.emit('sensor-data', { topic, data });
});

const PORT = 5000;
server.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
