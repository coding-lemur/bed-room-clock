# Bedroom Clock with ESP32

![final device](/docs/device.jpg)

## Description

You can put this device in your bedroom and you won't be disturbed by the light of the display. The display is only activated when you move your hand in front of the device and then it automatically goes off again.

## Features

- OLED display
- display turn on if you move your hand in front of it
- temperature and humidity sensor
- MQTT support (for Node-Red or Home Assistant)
- easy integration in own WiFi network (Hotspot settings-page)
- OTA updates

## Parts

- ESP32 DEV Board
- SSD1306 (I use a 2.42 inch version)
- HC-SR04
- DHT22

## Sketch

![sketch](/docs/beed-room-clock%20Sketch_bb.png)

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
