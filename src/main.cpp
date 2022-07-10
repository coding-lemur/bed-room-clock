#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SPIFFS.h>

#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <NewPing.h>
#include <DHT.h>
#include <time.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "config.h"

const String version = "1.1.0";

NewPing sonar(GPIO_NUM_5, GPIO_NUM_18);
Adafruit_SSD1306 ssd1306(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
DHT dht(DHT_PIN, DHT_TYPE);
AsyncWebServer server(80);

const char *ntpServer = "pool.ntp.org"; // TODO move to config
const long gmtOffset_sec = 3600;        // TODO move to config
const int daylightOffset_sec = 3600;    // TODO move to config

bool isPortalActive = false;
float lastTemperature = -100;
float lastHumidity = -100;
bool isTimeColonVisible = true;

unsigned long lastDisplayUpdate = 0;
unsigned long lastDhtUpdate = 0;
unsigned long lastDistanceUpdate = 0;
unsigned long lastScreenOn = 0;

double round2(double value)
{
  return (int)(value * 100 + 0.5) / 100.0;
}

// TODO move to library
String getDeviceId()
{
  int index = WiFiSettings.hostname.lastIndexOf('-');
  int length = WiFiSettings.hostname.length();
  return WiFiSettings.hostname.substring(index + 1, length);
}

// TODO move to library
int getRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100)
  {
    quality = 0;
  }
  else if (rssi >= -50)
  {
    quality = 100;
  }
  else
  {
    quality = 2 * (rssi + 100);
  }

  return quality;
}

StaticJsonDocument<384> getInfoJson()
{
  StaticJsonDocument<384> doc;
  doc["version"] = version;

  JsonObject system = doc.createNestedObject("system");
  system["deviceId"] = getDeviceId();
  system["freeHeap"] = ESP.getFreeHeap();                // in bytes
  system["uptime"] = esp_timer_get_time() / 1000 / 1000; // in seconds
  // system["time"] = NTP.getTimeDateStringForJS(); // getFormatedRtcNow();
  //   system["uptime"] = NTP.getUptimeString();

  // JsonObject fileSystem = doc.createNestedObject("fileSystem");
  // fileSystem["totalBytes"] = SPIFFS.totalBytes();
  // fileSystem["usedBytes"] = SPIFFS.usedBytes();

  JsonObject network = doc.createNestedObject("network");
  int8_t rssi = WiFi.RSSI();
  network["wifiRssi"] = rssi;
  network["wifiQuality"] = getRssiAsQuality(rssi);
  network["wifiSsid"] = WiFi.SSID();
  network["ip"] = WiFi.localIP().toString();
  network["mac"] = WiFi.macAddress();

  JsonObject values = doc.createNestedObject("values");

  if (lastTemperature > -100)
  {
    values["temp"] = round2(lastTemperature);
  }

  if (lastHumidity > -100)
  {
    values["humidity"] = round2(lastHumidity);
  }

  return doc;
}

void setupWebserver()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! This is a sample response."); });

  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        StringStream stream;
        auto size = serializeJson(getInfoJson(), stream);

        request->send(stream, "application/json", size); });

  AsyncElegantOTA.begin(&server);

  server.begin();
}

void setupDht()
{
  dht.begin();
}

void setupNtp()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// Start ArduinoOTA via WiFiSettings with the same hostname and password
void setupOta()
{
  ArduinoOTA.setHostname(WiFiSettings.hostname.c_str());
  ArduinoOTA.setPassword(WiFiSettings.password.c_str());
  ArduinoOTA.begin();
}

void setupDisplay()
{
  if (!ssd1306.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  delay(1000);

  ssd1306.setRotation(2); // flip screen
  ssd1306.clearDisplay();
  ssd1306.setTextColor(WHITE);
  ssd1306.setTextSize(1);

  ssd1306.setCursor(0, 0);
  ssd1306.print(">> BedRoom Clock <<");

  ssd1306.setCursor(0, 2);
  ssd1306.print(version);

  ssd1306.display();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  WiFi.reconnect();
}

void setupWifiSettings()
{
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);

  WiFiSettings.secure = true;
  WiFiSettings.hostname = "bedroom-"; // will auto add device ID
  WiFiSettings.password = PASSWORD;

  // Set callbacks to start OTA when the portal is active
  WiFiSettings.onPortal = []()
  {
    isPortalActive = true;

    Serial.println("WiFi config portal active");

    setupOta();

    ssd1306.clearDisplay();
    ssd1306.setTextSize(1);

    ssd1306.setCursor(0, 0);
    ssd1306.print("connect to WiFi and setup the device");

    ssd1306.setCursor(0, 10);
    ssd1306.print(WiFiSettings.hostname);

    ssd1306.setCursor(0, 20);
    ssd1306.print("password: ");
    ssd1306.print(PASSWORD);

    ssd1306.display();
  };
  WiFiSettings.onPortalWaitLoop = []()
  {
    ArduinoOTA.handle();
  };
  WiFiSettings.onConfigSaved = []()
  {
    ESP.restart();
  };
  WiFiSettings.onConnect = []()
  {
    ssd1306.clearDisplay();
    ssd1306.setTextSize(1);

    ssd1306.setCursor(0, 0);
    ssd1306.print("try to connect to WiFi");

    ssd1306.setCursor(0, 10);
    ssd1306.print(WiFi.SSID());

    ssd1306.display();
  };
  WiFiSettings.onSuccess = []()
  {
    ssd1306.clearDisplay();
    ssd1306.setTextSize(1);

    ssd1306.setCursor(0, 0);
    ssd1306.print("connected successfully to WiFi");

    ssd1306.setCursor(0, 10);
    ssd1306.print(WiFi.SSID());

    ssd1306.display();
  };
  WiFiSettings.onFailure = []()
  {
    ssd1306.clearDisplay();
    ssd1306.setTextSize(1);

    ssd1306.setCursor(0, 0);
    ssd1306.print("error on connecting to WiFi");

    ssd1306.setCursor(0, 10);
    ssd1306.print(WiFi.SSID());

    ssd1306.display();
  };

  // Use stored credentials to connect to your WiFi access point.
  // If no credentials are stored or if the access point is out of reach,
  // an access point will be started with a captive portal to configure WiFi.
  WiFiSettings.connect();
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  SPIFFS.begin(true); // Will format on the first run after failing to mount
  // SPIFFS.format();    // TODO reset config on connection MQTT fail

  setupDisplay();
  setupWifiSettings();
  setupWebserver();
  setupOta();
  setupNtp();
  setupDht();
}

void loop()
{
  ArduinoOTA.handle(); // If you also want the OTA during regular execution

  if (isPortalActive)
  {
    return;
  }

  bool shouldUpdateDhtValues = millis() - lastDhtUpdate >= DHT_UPDATE_INTERVAL;

  if (shouldUpdateDhtValues)
  {
    // wait a two seconds between measurements
    lastTemperature = dht.readTemperature();
    lastHumidity = dht.readHumidity();

    lastDhtUpdate = millis();
  }

  unsigned long lastDistance = sonar.ping_cm();
  if (lastDistance > 0 && lastDistance <= SCREEN_ON_DISTANCE)
  {
    // turn on display
    lastScreenOn = millis();
  }

  bool isDisplayOff = millis() - lastScreenOn >= SCREEN_ON_INTERVAL;
  bool shouldUpdateScreen = millis() - lastDisplayUpdate >= SCREEN_UPDATE_INTERVAL;

  // turn on/off screen
  uint8_t command = isDisplayOff ? SSD1306_DISPLAYOFF : SSD1306_DISPLAYON;
  ssd1306.ssd1306_command(command);

  if (!isDisplayOff && shouldUpdateScreen)
  {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      // TODO show error on display
      Serial.println("Failed to obtain time");
      return;
    }

    ssd1306.clearDisplay();

    // show time
    ssd1306.setTextSize(4);
    ssd1306.setCursor(3, 0);
    ssd1306.print(&timeinfo, "%H");

    ssd1306.print(isTimeColonVisible ? ":" : " ");
    isTimeColonVisible = !isTimeColonVisible;

    ssd1306.print(&timeinfo, "%M");
    /*ssd1306.print(":");
    ssd1306.print(&timeinfo, "%S");*/

    // show progressbar (for seconds value)
    int16_t pixel = (120 * timeinfo.tm_sec) / 60;
    ssd1306.fillRect(3, 35, pixel, 2, SSD1306_INVERSE);

    if (lastTemperature > -100)
    {
      // show temperature
      ssd1306.setTextSize(1);
      ssd1306.setCursor(ssd1306.width() - 50, ssd1306.height() - 10);
      ssd1306.print(lastTemperature);
      ssd1306.print(" C");
    }

    ssd1306.display();

    lastDisplayUpdate = millis();
  }
}