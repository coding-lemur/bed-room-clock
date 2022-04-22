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

### OLED Display (SSD1306 128x64)

| Pin on Sensor | description | Pin on ESP32      |
| ------------- | ----------- | ----------------- |
| CS            | SPI         | -                 |
| D/C           | SPI         | -                 |
| RES           | SPI         | -                 |
| SDA           | I2C         | GPIO 21 (I2C SDA) |
| SCL           | I2C         | GPIO 22 (I2C SCL) |
| 5V            |             | VIN (5V)          |
| GND           |             | GND               |

### DHT22

| Pin on Sensor | description | Pin on ESP32    |
| ------------- | ----------- | --------------- |
| 1             |             | 3V3 (3V)        |
| 2             |             | GPIO 4 (analog) |
| 3             |             | -               |
| 4             |             | GND             |
