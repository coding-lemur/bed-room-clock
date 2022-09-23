# Bedroom Clock with ESP32

![final device](/docs/device.jpg)

## Description

You can put this device in your bedroom and you won't be disturbed by the light of the display. The display is only activated when you move your hand in front of the device and then it automatically goes off again.

## Features

- OLED display
- display turn on if you move your hand in front of it
- temperature and humidity sensor
- easy integration in own WiFi network (Hotspot settings-page)
- vuejs 3 [dashboard](https://github.com/coding-lemur/bed-room-clock-dashboard)
- API endpoint
- OTA updates

## Demo

[See demo on Youtube](https://youtu.be/ncI8ftF5udI)

## Parts

- ESP32 DEV Board
- SSD1306 (I use a 2.42 inch version)
- HC-SR04
- DHT22

## Sketch

![sketch](/docs/beed-room-clock-sketch_bb.png)

## Wiring

### Ultra Sonic Sensor (HC-SR04)

| Pin on Sensor | description | Pin on ESP32 |
| ------------- | ----------- | ------------ |
| VCC           |             | VIN (5V)     |
| TRIG          |             | GPIO 5       |
| ECHO          |             | GPIO 18      |
| GND           |             | GND          |

### OLED Display (SSD1306 128x64) via SPI

| Pin on Sensor | description | Pin on ESP32 |
| ------------- | ----------- | ------------ |
| GND           |             | GND          |
| 5V            |             | VIN (5V)     |
| SCK/SCL       | SPI         | GPIO 14      |
| MOSI/SDA      | SPI         | GPIO 13      |
| RES/RST       | SPI         | GPIO 17      |
| DC            | SPI         | GPIO 16      |
| CS            | SPI         | GPIO 15      |

### DHT22

| Pin on Sensor | description | Pin on ESP32    |
| ------------- | ----------- | --------------- |
| 1             |             | 3V3 (3V)        |
| 2             |             | GPIO 4 (analog) |
| 3             |             | -               |
| 4             |             | GND             |

## API endpoint

### GET `/api/info`

Returning an JSON object with following data:

```json
{
  "version": "1.1.0",
  "system": {
    "deviceId": "xxyyzzz",
    "freeHeap": 236480,
    "uptime": 300 // in seconds
  },
  "network": {
    "wifiRssi": -59,
    "wifiQuality": 82,
    "wifiSsid": "wifi-name",
    "ip": "192.168.178.70",
    "mac": "C5:5F:4B:F0:64:29"
  },
  "values": {
    "temp": 24.6,
    "humidity": 42.7
  }
}
```

### POST `/api/restart`

Restart device

### POST `/api/hard-reset`

Removes all files (settings) from the device. Need setup for connecting to the WiFi
