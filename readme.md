# Bedroom Clock with ESP32

## Description

Show time

## Features

- OLED display
- display only on
- temperature and humidity sensor
- MQTT support (for Node-Red or Home Assistant)
- easy integration in own WiFi network (Hotspot settings-page)

## Parts

- later...

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

### OLED Display (SSD1306 128x64) via SPI ⚠

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
