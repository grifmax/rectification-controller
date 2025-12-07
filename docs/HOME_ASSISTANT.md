# Smart-Column S3 — Home Assistant Integration Guide

**Version:** 1.2+
**Last Updated:** 2025-12-06

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [MQTT Broker Setup](#mqtt-broker-setup)
4. [Device Configuration](#device-configuration)
5. [Home Assistant Configuration](#home-assistant-configuration)
6. [Dashboard Examples](#dashboard-examples)
7. [Automations](#automations)
8. [Energy Dashboard](#energy-dashboard)
9. [Troubleshooting](#troubleshooting)

---

## Overview

Smart-Column S3 integrates seamlessly with Home Assistant using MQTT Discovery protocol. Once configured, all sensors and entities are automatically discovered and ready to use.

### Features

- 🔍 **Automatic Discovery** - No manual entity configuration needed
- 📊 **Real-time Monitoring** - Temperature, power, energy tracking
- ⚡ **Energy Dashboard** - Track energy consumption over time
- 🚨 **Automations** - Alerts and automatic actions
- 📱 **Mobile Access** - Monitor from anywhere via HA app

---

## Prerequisites

### Required

- Home Assistant installed and running
- MQTT Broker (Mosquitto recommended)
- Smart-Column S3 device on same network
- Network connectivity between HA and device

### Optional

- Telegram bot for notifications
- Node-RED for advanced automations

---

## MQTT Broker Setup

### Option 1: Mosquitto Add-on (Recommended)

1. **Install Mosquitto Broker**
   - Navigate to: Settings → Add-ons → Add-on Store
   - Search for "Mosquitto broker"
   - Click Install

2. **Configure Mosquitto**

   Add-on Configuration:
   ```yaml
   logins:
     - username: mqtt_user
       password: your_secure_password
   require_certificate: false
   certfile: fullchain.pem
   keyfile: privkey.pem
   ```

3. **Start the Add-on**
   - Enable "Start on boot"
   - Click "START"

4. **Verify Installation**
   - Check the Log tab for successful startup

### Option 2: External Mosquitto

If running Mosquitto externally:

```bash
# Install Mosquitto
sudo apt-get install mosquitto mosquitto-clients

# Create password file
sudo mosquitto_passwd -c /etc/mosquitto/passwd mqtt_user

# Edit config
sudo nano /etc/mosquitto/mosquitto.conf
```

Add:
```
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Restart:
```bash
sudo systemctl restart mosquitto
```

---

## Device Configuration

### Access Web Interface

1. Open browser: `http://<device-ip>`
2. Navigate to Settings → MQTT
3. Configure as follows:

### MQTT Settings

| Setting | Value | Example |
|---------|-------|---------|
| **Enable MQTT** | ☑ Enabled | |
| **MQTT Server** | HA IP or hostname | `192.168.1.50` or `homeassistant.local` |
| **MQTT Port** | Default: 1883 | `1883` |
| **Username** | From Mosquitto config | `mqtt_user` |
| **Password** | From Mosquitto config | `your_secure_password` |
| **Base Topic** | Leave default | `smartcolumn` |
| **Enable Discovery** | ☑ Enabled | |
| **Publish Interval** | 10000 ms (10 sec) | `10000` |

4. Click **Save Settings**
5. Device will restart and connect to MQTT broker

### Verify Connection

Check device logs:
```
[MQTT] Connecting to 192.168.1.50:1883...
[MQTT] Connected successfully
[MQTT] Publishing discovery messages...
[MQTT] Discovery complete - 6 entities published
```

---

## Home Assistant Configuration

### Add MQTT Integration

1. **Navigate to Integrations**
   - Settings → Devices & Services → Integrations

2. **Add MQTT**
   - Click "+ ADD INTEGRATION"
   - Search for "MQTT"
   - Select "MQTT"

3. **Configure Connection**
   ```
   Broker: localhost (if using add-on) or IP address
   Port: 1883
   Username: mqtt_user
   Password: your_secure_password
   ```

4. **Enable Discovery**
   - Discovery prefix: `homeassistant` (default)
   - ☑ Enable newly added entities

5. **Submit**

### Verify Auto-Discovery

After device connects, check:

**Settings → Devices & Services → MQTT**

You should see a new device:
```
Smart Column abc123
  - Cube Temperature
  - Column Top Temperature
  - Voltage
  - Current
  - Power
  - Energy
  - System Health
```

If not visible, check [Troubleshooting](#troubleshooting).

---

## Dashboard Examples

### Simple Status Card

```yaml
type: entities
title: Smart Column Status
entities:
  - entity: sensor.smart_column_abc123_cube_temp
    name: Cube Temperature
  - entity: sensor.smart_column_abc123_column_top
    name: Column Top
  - entity: sensor.smart_column_abc123_voltage
    name: Voltage
  - entity: sensor.smart_column_abc123_power
    name: Power
  - entity: sensor.smart_column_abc123_health
    name: System Health
```

### Temperature Gauge

```yaml
type: gauge
entity: sensor.smart_column_abc123_cube_temp
name: Cube Temperature
min: 0
max: 100
severity:
  green: 0
  yellow: 70
  red: 90
needle: true
```

### Power Monitoring

```yaml
type: vertical-stack
cards:
  - type: gauge
    entity: sensor.smart_column_abc123_power
    name: Current Power
    min: 0
    max: 5000
    unit: W
    severity:
      green: 0
      yellow: 2500
      red: 4000

  - type: sensor
    entity: sensor.smart_column_abc123_energy
    name: Total Energy
    graph: line
    detail: 2
```

### Historical Graph

```yaml
type: history-graph
title: Temperature History
hours_to_show: 24
entities:
  - entity: sensor.smart_column_abc123_cube_temp
    name: Cube
  - entity: sensor.smart_column_abc123_column_top
    name: Column Top
```

### Complete Dashboard

```yaml
type: vertical-stack
title: 🥃 Smart Column Monitor
cards:
  # Status Overview
  - type: horizontal-stack
    cards:
      - type: gauge
        entity: sensor.smart_column_abc123_cube_temp
        name: Cube
        min: 0
        max: 100
        severity:
          green: 0
          yellow: 70
          red: 90

      - type: gauge
        entity: sensor.smart_column_abc123_health
        name: Health
        min: 0
        max: 100
        severity:
          red: 0
          yellow: 50
          green: 80

  # Power Monitoring
  - type: horizontal-stack
    cards:
      - type: entity
        entity: sensor.smart_column_abc123_voltage
        name: Voltage
        icon: mdi:flash

      - type: entity
        entity: sensor.smart_column_abc123_current
        name: Current
        icon: mdi:current-ac

      - type: entity
        entity: sensor.smart_column_abc123_power
        name: Power
        icon: mdi:lightning-bolt

  # Temperature History
  - type: history-graph
    title: Temperature Trends
    hours_to_show: 12
    entities:
      - sensor.smart_column_abc123_cube_temp
      - sensor.smart_column_abc123_column_top

  # Energy Consumption
  - type: energy-date-selection
  - type: energy-sources-table
```

---

## Automations

### Alert on High Temperature

```yaml
alias: Smart Column - High Temperature Alert
trigger:
  - platform: numeric_state
    entity_id: sensor.smart_column_abc123_cube_temp
    above: 95
condition: []
action:
  - service: notify.mobile_app
    data:
      title: ⚠️ Smart Column Alert
      message: "Cube temperature critical: {{ states('sensor.smart_column_abc123_cube_temp') }}°C"
      data:
        priority: high
        ttl: 0
mode: single
```

### Low System Health Warning

```yaml
alias: Smart Column - Low Health Warning
trigger:
  - platform: numeric_state
    entity_id: sensor.smart_column_abc123_health
    below: 70
condition: []
action:
  - service: notify.persistent_notification
    data:
      title: Smart Column Health Low
      message: "System health at {{ states('sensor.smart_column_abc123_health') }}%. Please check device."
  - service: persistent_notification.create
    data:
      title: ⚠️ Smart Column
      message: "Health: {{ states('sensor.smart_column_abc123_health') }}%"
mode: single
```

### Telegram Notification on Process Complete

```yaml
alias: Smart Column - Process Complete
trigger:
  - platform: state
    entity_id: sensor.smart_column_abc123_cube_temp
    to: "25.0"  # Temperature returns to room temp
    for:
      minutes: 10
condition:
  - condition: template
    value_template: "{{ states('sensor.smart_column_abc123_cube_temp') | float < 30 }}"
action:
  - service: telegram_bot.send_message
    data:
      message: |
        ✅ Rectification process completed!

        Final Stats:
        🌡️ Cube: {{ states('sensor.smart_column_abc123_cube_temp') }}°C
        ⚡ Energy used: {{ states('sensor.smart_column_abc123_energy') }} kWh

        Device is cooling down.
mode: single
```

### Power Limit Protection

```yaml
alias: Smart Column - Power Limit
trigger:
  - platform: numeric_state
    entity_id: sensor.smart_column_abc123_power
    above: 4500
condition: []
action:
  - service: notify.mobile_app
    data:
      title: ⚠️ Power Limit Exceeded
      message: "Current: {{ states('sensor.smart_column_abc123_power') }}W"
      data:
        priority: high
        actions:
          - action: STOP_COLUMN
            title: Stop Device
mode: single
```

### Daily Energy Report

```yaml
alias: Smart Column - Daily Report
trigger:
  - platform: time
    at: "23:00:00"
condition:
  - condition: template
    value_template: "{{ states('sensor.smart_column_abc123_energy') | float > 0 }}"
action:
  - service: notify.telegram
    data:
      message: |
        📊 Smart Column Daily Report

        ⚡ Energy today: {{ states('sensor.smart_column_abc123_energy') }} kWh
        🌡️ Max temp: {{ state_attr('sensor.smart_column_abc123_cube_temp', 'max_value') }}°C

        {{ now().strftime('%Y-%m-%d') }}
mode: single
```

---

## Energy Dashboard

### Setup

1. **Navigate to Energy Dashboard**
   - Settings → Dashboards → Energy

2. **Add Electricity Grid**
   - Click "Add Consumption"
   - Select "Individual Devices"

3. **Select Smart Column Energy Sensor**
   - Choose: `sensor.smart_column_abc123_energy`
   - Unit: kWh
   - Click "Save"

4. **Configure Cost (Optional)**
   - Set your electricity rate
   - Example: €0.15/kWh

### Energy Tracking

The device reports cumulative energy consumption. The sensor uses `state_class: total_increasing` which is perfect for Energy Dashboard.

**Features:**
- Daily/Monthly/Yearly consumption graphs
- Cost calculation
- Compare with other devices
- Export data to CSV

### Advanced: Per-Process Energy Tracking

Create utility meters for process-specific tracking:

```yaml
utility_meter:
  smart_column_process_energy:
    source: sensor.smart_column_abc123_energy
    cycle: none  # Manual reset

  smart_column_daily:
    source: sensor.smart_column_abc123_energy
    cycle: daily
```

Automation to reset per-process meter:

```yaml
alias: Smart Column - Reset Process Energy
trigger:
  - platform: state
    entity_id: sensor.smart_column_abc123_power
    from: "0"
    to: "0"
    for:
      hours: 1
action:
  - service: utility_meter.reset
    target:
      entity_id: utility_meter.smart_column_process_energy
```

---

## Troubleshooting

### Device Not Discovered

**Check MQTT connection:**
1. Device Web UI → Status
2. Look for "MQTT: Connected"

**Check Home Assistant MQTT logs:**
```
Settings → System → Logs
Filter: mqtt
```

**Manually listen to topics:**
```bash
mosquitto_sub -h localhost -u mqtt_user -P password -t 'smartcolumn/#' -v
```

Expected output:
```
smartcolumn/abc123/state {"mode":0,"temperatures":{...}}
smartcolumn/abc123/health 95
smartcolumn/abc123/status online
```

### Entities Show "Unavailable"

**Possible causes:**

1. **Device offline**
   - Check device network connectivity
   - Ping device IP

2. **MQTT broker down**
   - Settings → Add-ons → Mosquitto broker
   - Check status and logs

3. **Wrong credentials**
   - Verify username/password match
   - Check Mosquitto logs for auth failures

### Discovery Messages Not Received

**Force republish:**

1. Device Web UI → Settings → MQTT
2. Uncheck "Enable Discovery"
3. Save
4. Check "Enable Discovery"
5. Save

Device will republish all discovery messages.

**Manual discovery check:**

```bash
mosquitto_sub -h localhost -u mqtt_user -P password -t 'homeassistant/#' -v
```

Should see messages like:
```
homeassistant/sensor/abc123_cube_temp/config {"name":"Cube Temperature",...}
```

### Energy Values Not Updating

**Verify energy sensor:**
- Developer Tools → States
- Find: `sensor.smart_column_abc123_energy`
- Check `state_class: total_increasing`

**Check Energy Dashboard settings:**
- Energy → Settings
- Verify sensor is added
- Check unit is kWh

**Force update:**
- Device Web UI → Reboot
- Wait for reconnection
- Check Energy Dashboard after 1 minute

### High MQTT Traffic

**Reduce publish interval:**

Device Web UI → Settings → MQTT:
- Increase "Publish Interval"
- Recommended: 10000-30000 ms
- Save

**QoS Settings:**

For lower priority data, use QoS 0 instead of QoS 1.

---

## Best Practices

### Performance

- Use publish interval ≥ 10 seconds
- Enable discovery only when needed
- Use QoS 0 for high-frequency data

### Security

- Use strong MQTT passwords
- Enable authentication on broker
- Use TLS for external access
- Keep firmware updated

### Reliability

- Enable "Start on boot" for Mosquitto
- Use persistent MQTT sessions
- Configure LWT for availability tracking
- Regular device reboots (monthly)

### Monitoring

- Create health check automation
- Monitor energy consumption trends
- Set up alerts for critical values
- Log important events

---

## Additional Resources

### Official Docs
- [Home Assistant MQTT](https://www.home-assistant.io/integrations/mqtt/)
- [MQTT Discovery](https://www.home-assistant.io/docs/mqtt/discovery/)
- [Energy Dashboard](https://www.home-assistant.io/docs/energy/)

### Community
- [Home Assistant Forums](https://community.home-assistant.io/)
- [r/homeassistant](https://reddit.com/r/homeassistant)

### Tools
- [MQTT Explorer](http://mqtt-explorer.com/) - Desktop MQTT client
- [Node-RED](https://nodered.org/) - Visual automation flows

---

## Example: Complete Setup Script

For advanced users, here's a complete setup using `configuration.yaml`:

```yaml
# MQTT Configuration
mqtt:
  broker: localhost
  port: 1883
  username: !secret mqtt_user
  password: !secret mqtt_password
  discovery: true
  discovery_prefix: homeassistant
  birth_message:
    topic: 'hass/status'
    payload: 'online'
  will_message:
    topic: 'hass/status'
    payload: 'offline'

# Utility Meters for Energy Tracking
utility_meter:
  smart_column_daily_energy:
    source: sensor.smart_column_abc123_energy
    cycle: daily

  smart_column_monthly_energy:
    source: sensor.smart_column_abc123_energy
    cycle: monthly

# Template Sensors
template:
  - sensor:
      - name: "Smart Column Status"
        state: >
          {% if states('sensor.smart_column_abc123_power') | float > 100 %}
            Running
          {% else %}
            Idle
          {% endif %}
        icon: >
          {% if states('sensor.smart_column_abc123_power') | float > 100 %}
            mdi:flask
          {% else %}
            mdi:flask-empty-outline
          {% endif %}

      - name: "Smart Column Efficiency"
        unit_of_measurement: "%"
        state: >
          {{ states('sensor.smart_column_abc123_health') }}

# Automations
automation:
  - alias: Smart Column Alert - High Temp
    trigger:
      - platform: numeric_state
        entity_id: sensor.smart_column_abc123_cube_temp
        above: 95
    action:
      - service: notify.mobile_app
        data:
          title: "⚠️ High Temperature"
          message: "{{ states('sensor.smart_column_abc123_cube_temp') }}°C"
```

---

*Last updated: 2025-12-06*
*For support: Check device logs or open GitHub issue*
