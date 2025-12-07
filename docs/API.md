# Smart-Column S3 — API Documentation

**Version:** 1.2+
**Last Updated:** 2025-12-06

## Table of Contents

1. [REST API](#rest-api)
2. [WebSocket API](#websocket-api)
3. [MQTT Integration](#mqtt-integration)
4. [Authentication](#authentication)
5. [Rate Limiting](#rate-limiting)
6. [Error Codes](#error-codes)
7. [Examples](#examples)

---

## REST API

All REST API endpoints are available at `http://<device-ip>/api/`

### Authentication

Most endpoints require HTTP Basic Authentication (if enabled in settings).

```http
Authorization: Basic <base64(username:password)>
```

### Endpoints

#### GET /api/status

Get current system status.

**Response:**
```json
{
  "mode": 0,
  "phase": 1,
  "temperatures": {
    "cube": 78.5,
    "column_bottom": 75.2,
    "column_top": 82.1,
    "reflux": 80.5,
    "tsa": 45.2,
    "water_in": 18.5,
    "water_out": 25.3
  },
  "power": {
    "voltage": 220.5,
    "current": 4.25,
    "power": 937,
    "energy": 2.456
  },
  "heater_power": 85,
  "pump_speed": 120,
  "uptime": 145620
}
```

#### GET /api/health

Get system health status (0-100%).

**Response:**
```json
{
  "health": 95,
  "checks": {
    "temperatures": true,
    "power": true,
    "water_flow": true,
    "pressure": true
  },
  "warnings": [],
  "errors": []
}
```

#### GET /api/sensors

Get raw sensor readings.

**Response:**
```json
{
  "ds18b20": [78.5, 75.2, 82.1, 80.5, 45.2, 18.5, 25.3],
  "bmp280_1": {"temperature": 22.5, "pressure": 101325},
  "bmp280_2": {"temperature": 78.2, "pressure": 102100},
  "mpx5010": 850,
  "zmpt101b": 220.5,
  "acs712": 4.25,
  "yf_s201": 12.5
}
```

#### POST /api/mode/start

Start a specific mode.

**Request:**
```json
{
  "mode": 0,
  "params": {
    "heads_percent": 8,
    "body_speed": 100,
    "target_temp": 78.5
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Rectification mode started"
}
```

#### POST /api/mode/stop

Stop current operation.

**Response:**
```json
{
  "success": true,
  "message": "Operation stopped"
}
```

#### POST /api/heater/power

Set heater power (0-100%).

**Request:**
```json
{
  "power": 85
}
```

**Response:**
```json
{
  "success": true,
  "power": 85
}
```

#### POST /api/pump/speed

Set pump speed (ml/hour).

**Request:**
```json
{
  "speed": 120
}
```

**Response:**
```json
{
  "success": true,
  "speed": 120
}
```

#### GET /api/calibration

Get calibration data.

**Response:**
```json
{
  "ds18b20_offsets": [0.0, -0.2, 0.1, 0.0, -0.1, 0.0, 0.2],
  "pump_ml_per_rev": 2.5,
  "voltage_calibration": 1.0,
  "current_calibration": 1.0,
  "pressure_calibration": 1.0
}
```

#### POST /api/calibration

Update calibration data.

**Request:**
```json
{
  "ds18b20_offsets": [0.0, -0.2, 0.1, 0.0, -0.1, 0.0, 0.2],
  "pump_ml_per_rev": 2.5
}
```

**Response:**
```json
{
  "success": true,
  "message": "Calibration updated"
}
```

#### GET /api/system/info

Get system information.

**Response:**
```json
{
  "firmware_version": "1.2.1",
  "hardware": "ESP32-S3 DevKitC-1 N16R8",
  "uptime": 145620,
  "free_heap": 245760,
  "wifi_rssi": -45,
  "ip_address": "192.168.1.100"
}
```

#### POST /api/system/reboot

Reboot the device.

**Response:**
```json
{
  "success": true,
  "message": "Rebooting in 3 seconds"
}
```

---

## WebSocket API

Connect to WebSocket at `ws://<device-ip>/ws`

### Message Format

All messages are JSON formatted.

#### Server → Client (Status Updates)

Sent every 1 second during operation.

```json
{
  "type": "status",
  "data": {
    "mode": 0,
    "phase": 1,
    "temperatures": { ... },
    "power": { ... },
    "heater_power": 85,
    "pump_speed": 120
  }
}
```

#### Server → Client (Events)

```json
{
  "type": "event",
  "event": "phase_change",
  "data": {
    "from": 1,
    "to": 2,
    "phase_name": "HEADS"
  }
}
```

#### Client → Server (Commands)

```json
{
  "command": "set_power",
  "value": 85
}
```

```json
{
  "command": "set_speed",
  "value": 120
}
```

```json
{
  "command": "start_mode",
  "mode": 0,
  "params": { ... }
}
```

---

## MQTT Integration

### Connection

```
Server: <mqtt_broker>
Port: 1883 (default)
Username: <mqtt_username> (optional)
Password: <mqtt_password> (optional)
```

### Topics Structure

#### State Topic
```
smartcolumn/<device_id>/state
```

**Payload Example:**
```json
{
  "mode": 0,
  "phase": 1,
  "temperatures": {
    "cube": 78.5,
    "column_top": 82.1
  },
  "power": {
    "voltage": 220.5,
    "current": 4.25,
    "power": 937,
    "energy": 2.456
  }
}
```

**Published:** Every 10 seconds (configurable)

#### Health Topic
```
smartcolumn/<device_id>/health
```

**Payload:** `95` (percentage 0-100)
**Published:** Every 5 seconds

#### Availability Topic
```
smartcolumn/<device_id>/status
```

**Payload:** `online` or `offline`
**Retained:** Yes
**LWT:** Yes

### Home Assistant MQTT Discovery

The device automatically publishes discovery messages to Home Assistant.

#### Discovery Topic Format
```
homeassistant/sensor/<device_id>_<entity>/config
```

#### Example Discovery Message (Temperature Sensor)

**Topic:** `homeassistant/sensor/abc123_cube_temp/config`

**Payload:**
```json
{
  "name": "Cube Temperature",
  "uniq_id": "abc123_cube_temp",
  "stat_t": "smartcolumn/abc123/state",
  "val_tpl": "{{ value_json.temperatures.cube }}",
  "unit_of_meas": "°C",
  "dev_cla": "temperature",
  "avty_t": "smartcolumn/abc123/status",
  "dev": {
    "ids": ["smartcolumn_abc123"],
    "name": "Smart Column abc123",
    "mdl": "ESP32-S3 DevKitC-1",
    "mf": "DIY",
    "sw": "1.2.1"
  }
}
```

#### Available Entities

| Entity | Device Class | Unit | State Class |
|--------|--------------|------|-------------|
| Cube Temperature | temperature | °C | measurement |
| Column Top Temperature | temperature | °C | measurement |
| Voltage | voltage | V | measurement |
| Current | current | A | measurement |
| Power | power | W | measurement |
| Energy | energy | kWh | total_increasing |
| System Health | None | % | measurement |

---

## Authentication

### HTTP Basic Authentication

When authentication is enabled, all HTTP requests must include:

```http
Authorization: Basic <base64(username:password)>
```

### Example (Python)
```python
import requests
from requests.auth import HTTPBasicAuth

response = requests.get(
    'http://192.168.1.100/api/status',
    auth=HTTPBasicAuth('admin', 'password')
)
```

### Example (cURL)
```bash
curl -u admin:password http://192.168.1.100/api/status
```

### Example (JavaScript)
```javascript
fetch('http://192.168.1.100/api/status', {
  headers: {
    'Authorization': 'Basic ' + btoa('admin:password')
  }
})
```

---

## Rate Limiting

The system implements rate limiting to prevent abuse.

**Limits:**
- 60 requests per minute per IP address
- Applies to all HTTP endpoints

**Response when exceeded:**
```http
HTTP/1.1 429 Too Many Requests
Content-Type: application/json

{
  "error": "Rate limit exceeded",
  "retry_after": 45
}
```

---

## Error Codes

| HTTP Code | Meaning |
|-----------|---------|
| 200 | Success |
| 400 | Bad Request - Invalid parameters |
| 401 | Unauthorized - Authentication required or failed |
| 404 | Not Found - Endpoint doesn't exist |
| 429 | Too Many Requests - Rate limit exceeded |
| 500 | Internal Server Error |

### Error Response Format

```json
{
  "error": "Error message",
  "code": 400,
  "details": "Additional information"
}
```

---

## Examples

### Python - Get Status and Control

```python
import requests
from requests.auth import HTTPBasicAuth
import time

BASE_URL = 'http://192.168.1.100'
auth = HTTPBasicAuth('admin', 'password')

# Get current status
status = requests.get(f'{BASE_URL}/api/status', auth=auth).json()
print(f"Current temperature: {status['temperatures']['cube']}°C")

# Set heater power
requests.post(
    f'{BASE_URL}/api/heater/power',
    json={'power': 75},
    auth=auth
)

# Start rectification mode
requests.post(
    f'{BASE_URL}/api/mode/start',
    json={
        'mode': 0,
        'params': {
            'heads_percent': 8,
            'body_speed': 100
        }
    },
    auth=auth
)

# Monitor status
while True:
    status = requests.get(f'{BASE_URL}/api/status', auth=auth).json()
    print(f"Cube: {status['temperatures']['cube']}°C, Power: {status['power']['power']}W")
    time.sleep(5)
```

### Node.js - MQTT Subscribe

```javascript
const mqtt = require('mqtt');

const client = mqtt.connect('mqtt://localhost:1883', {
  username: 'mqtt_user',
  password: 'mqtt_pass'
});

client.on('connect', () => {
  // Subscribe to all topics for device
  client.subscribe('smartcolumn/abc123/#');
});

client.on('message', (topic, message) => {
  console.log(`${topic}: ${message.toString()}`);

  if (topic.endsWith('/state')) {
    const state = JSON.parse(message.toString());
    console.log(`Cube temp: ${state.temperatures.cube}°C`);
    console.log(`Power: ${state.power.power}W`);
  }

  if (topic.endsWith('/health')) {
    const health = parseInt(message.toString());
    console.log(`System health: ${health}%`);
    if (health < 80) {
      console.warn('Warning: Low system health!');
    }
  }
});
```

### Home Assistant - Energy Dashboard

Add to `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "Smart Column Energy"
      state_topic: "smartcolumn/abc123/state"
      value_template: "{{ value_json.power.energy }}"
      unit_of_measurement: "kWh"
      device_class: energy
      state_class: total_increasing
```

Then add to Energy Dashboard:
1. Settings → Dashboards → Energy
2. Add Energy Source
3. Select "Smart Column Energy"

---

## Security Headers

All HTTP responses include security headers:

```http
Content-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline'
X-Frame-Options: DENY
X-Content-Type-Options: nosniff
X-XSS-Protection: 1; mode=block
Referrer-Policy: strict-origin-when-cross-origin
Permissions-Policy: geolocation=(), microphone=(), camera=()
```

---

## Changelog

### v1.2.1 (2025-12-06)
- Added MQTT integration
- Added Home Assistant MQTT Discovery
- Added HTTP Basic Authentication
- Added Rate Limiting (60 req/min)
- Added Security Headers
- Added `/api/health` endpoint

### v1.2.0 (2025-12-05)
- Added PZEM-004T power monitoring
- Added energy tracking
- Added WebSocket health updates
- Added Telegram alerts

### v1.1.0 (2025-12-02)
- Initial API documentation
- Core REST endpoints
- WebSocket support

---

*Last updated: 2025-12-06*
