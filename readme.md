# Bedroom Clock with ESP32 ⏰

![final device](/docs/device.webp)

## Description

You can put this device in your bedroom and you won't be disturbed by the light of the display. The display is only activated when you move your hand in front of the device and then it automatically goes off again.

## Features

- OLED display
- display turn on if you move your hand in front of it
- temperature and humidity sensor
- easy integration in own WiFi network (Hotspot settings-page)
- [react dashboard](https://github.com/coding-lemur/bed-room-clock-dashboard)
- REST API endpoint
- OTA updates

## Demo

[See demo on Youtube](https://youtu.be/ncI8ftF5udI)

## Parts

- ESP32 DEV Board
- SSD1306 (I use a 2.42 inch version)
- HC-SR04
- DHT22
- MAX 98357A (audio module)
- speaker

## Sketch

![sketch](/docs/beed-room-clock-sketch_bb.png)

## Wiring

### Ultra Sonic Sensor (HC-SR04)

| Pin on Sensor | Pin on ESP32 |
| ------------- | ------------ |
| VCC           | VIN (5V)     |
| TRIG          | GPIO 5       |
| ECHO          | GPIO 18      |
| GND           | GND          |

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

| Pin on Sensor  | Pin on ESP32    |
| -------------  | --------------- |
| 1              | 3V3 (3V)        |
| 2              | GPIO 4 (analog) |
| 3              | -               |
| 4              | GND             |

### MAX 98357A (Audio Interface)

| Pin on Sensor  | Pin on ESP32    |
| -------------  | --------------- |
| LRC            | GPIO 25         |
| BCLK           | GPIO 26         |
| DIN            | GPIO 22         |
| GAIN           | -               |
| SD             | -               |
| GND            | GND             |
| Vin            | Vin (5V)        |

## Timezone configuration

First change variable `timeZone` to your timezone. See [timezone list](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv) for the correct value.

Later I will include this setting also to the dashboard.

## REST API endpoint

### General

#### Get info

```http
GET /api/info
```

Returning an JSON object with following data:

| field | type | description |
| ------| -----| ------------|
| version      | string     | firmware version number |
| ------| -----| ------------|
| system.deviceId      | string     | unique ID of the device |
| system.freeHeap      | number    | the free heap in bytes |
| system.uptime      | number    |  uptime since last restart of the device in seconds |
| ------| -----| ------------|
| fileSystem.totalBytes      | number    | total bytes of the FS |
| fileSystem.usedBytes      | number    | used bytes of the FS |
| ------| -----| ------------|
| network.wifiRssi      | number    |   |
| network.wifiQuality      | number    | quality of the WiFi connection in percent  |
| network.wifiSsid      | string    | SSID of the current connected WiFi  |
| network.ip      | string    | IP address of the device  |
| network.mac      | string    | MAC address of the device  |
| ------| -----| ------------|
| values.temp      | number    | temperature in °C  |
| values.humidity      | number    | humidity in percent  |
| ------| -----| ------------|
| player.isPlaying      | boolean    | TRUE: if a stream is currently playing  |
| player.codec      | string    | name of the codec of the current stream (e.g. "MP3")  |
| player.bitrate      | number    | bitrate of the current stream  |
| player.volume      | number (0..21)    | volume of the current stream  |

#### Get settings

```http
GET /api/settings
```

Return the settings.json

#### Change settings

```http
POST /api/settings
```

Change settings.
Send payload as JSON in body.

| field            | type             | default | description                                                                                                   |
| ---------------- | ---------------- | ------- | ------------------------------------------------------------------------------------------------------------- |
| brightness       | number (0 - 255) | 255     | The brightness of the display.                                                                                |
| screenOnDistance | number (0 - 255) | 18 cm   | Specifies the distance from the ultrasonic sensor in centimeters from when the display should be switched on. |
| screenOnInterval | number           | 8000 ms | Specifies the time in milliseconds that the display stays on after motion detection.                          |

#### Restart device

```http
POST /api/restart
```

#### Factory reset

```http
POST /api/hard-reset
```

Reset device to factory settings.

**Warning**: all files (~settings) will removed from the device. Need setup for connecting to your WiFi.

### Audio-Player

#### Start Stream

```http
POST /api/player/start
```

Payload

| field            | type             | default | description                                                                                                   |
| ---------------- | ---------------- | ------- | ------------------------------------------------------------------------------------------------------------- |
| source                                                                                | string | | URL of the streaming source (MP3, OGG, AAC, FLAC)
| volume | number (between 0 and 11) |    | volume level |

#### Change Volume

```http
POST /api/player/volume
```

Payload

| field            | type             | default | description                                                                                                   |
| ---------------- | ---------------- | ------- | ------------------------------------------------------------------------------------------------------------- |
| volume | number (between 0 and 11) |    | volume level |

#### Stop Stream

```http
POST /api/player/stop
```

## Roadmap

- [ ] SD Card support (for storing audio files)
- [ ] support of input button(s)
- [ ] extend REST API to turn on the display
- [ ] Move all settings to the dashboard
