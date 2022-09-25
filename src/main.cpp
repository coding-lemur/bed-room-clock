#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SPIFFS.h>

// sensors
#include <NewPing.h>
#include <DHT.h>

// display
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>

#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

// webserver
#include <AsyncTCP.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

#include <AsyncElegantOTA.h>
#include <ESPAsyncTunnel.h>
#include <ESPmDNS.h>

#include "config.h"

const String version = "1.4.0";
const char *settingsFilename = "/settings.json";

NewPing sonar(GPIO_NUM_5, GPIO_NUM_18);
Adafruit_SSD1306 ssd1306(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
DHT dht(DHT_PIN, DHT_TYPE);
AsyncWebServer server(80);

const char *ntpServer = "pool.ntp.org"; // TODO move to config
const long gmtOffset_sec = 3600;        // TODO move to config
const int daylightOffset_sec = 3600;    // TODO move to config

// TODO move to settings
const char *externalBaseUrl = "https://coding-lemur.github.io";
const char *indexPath = "/bed-room-clock-dashboard/index.html";

bool isPortalActive = false;
float lastTemperature = -100;
float lastHumidity = -100;
bool isTimeColonVisible = true;

// settings
byte brightness = 255;
byte screenOnDistance = 18;                // in cm
unsigned long screenOnInterval = 8 * 1000; // in ms

unsigned long lastDisplayUpdate = 0;
unsigned long lastDhtUpdate = 0;
unsigned long lastDistanceUpdate = 0;
unsigned long lastScreenOn = 0;

// TODO move to library
double round2(double value)
{
  return (int)(value * 100 + 0.5) / 100.0;
}

void saveSettings()
{
  StaticJsonDocument<384> settings;
  settings["brightness"] = brightness;
  settings["screenOnDistance"] = screenOnDistance;
  settings["screenOnInterval"] = screenOnInterval;

  File file = SPIFFS.open(settingsFilename, FILE_WRITE);

  if (serializeJson(settings, file) == 0)
  {
    Serial.println("error on writing 'settings.json'");
  }

  file.close();
}

void loadSettings()
{
  if (!SPIFFS.exists(settingsFilename))
  {
    return;
  }

  File file = SPIFFS.open(settingsFilename, FILE_READ);
  StaticJsonDocument<384> settings;
  auto error = deserializeJson(settings, file);
  file.close();

  if (error)
  {
    Serial.println("error on deserializing 'auto-starts' file: ");
    return;
  }

  if (settings.containsKey("brightness"))
  {
    brightness = settings["brightness"].as<byte>();
  }

  if (settings.containsKey("screenOnDistance"))
  {
    screenOnDistance = settings["screenOnDistance"].as<byte>();
  }

  if (settings.containsKey("screenOnInterval"))
  {
    screenOnInterval = settings["screenOnInterval"].as<unsigned long>();
  }

  setBrightness();
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

// TODO move to library
unsigned long getUnixTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return 0;
  }

  time(&now);
  return now; // unix time
}

StaticJsonDocument<384> getInfoJson()
{
  StaticJsonDocument<384> doc;
  doc["version"] = version;

  JsonObject systemPart = doc.createNestedObject("system");
  systemPart["deviceId"] = getDeviceId();
  systemPart["freeHeap"] = ESP.getFreeHeap();                // in bytes
  systemPart["uptime"] = esp_timer_get_time() / 1000 / 1000; // in seconds
  systemPart["time"] = getUnixTime();

  JsonObject fileSystemPart = doc.createNestedObject("fileSystem");
  fileSystemPart["totalBytes"] = SPIFFS.totalBytes();
  fileSystemPart["usedBytes"] = SPIFFS.usedBytes();

  JsonObject networkPart = doc.createNestedObject("network");
  int8_t rssi = WiFi.RSSI();
  networkPart["wifiRssi"] = rssi;
  networkPart["wifiQuality"] = getRssiAsQuality(rssi);
  networkPart["wifiSsid"] = WiFi.SSID();
  networkPart["ip"] = WiFi.localIP().toString();
  networkPart["mac"] = WiFi.macAddress();

  JsonObject valuesPart = doc.createNestedObject("values");

  if (lastTemperature > -100)
  {
    valuesPart["temp"] = round2(lastTemperature);
  }

  if (lastHumidity > -100)
  {
    valuesPart["humidity"] = round2(lastHumidity);
  }

  return doc;
}

void setBrightness()
{
  ssd1306.ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306.ssd1306_command(brightness); // 0-255
}

void onChangeSettings(AsyncWebServerRequest *request, JsonVariant &json)
{
  StaticJsonDocument<200> data = json.as<JsonObject>();

  bool isDirty = false;

  if (data.containsKey("brightness"))
  {
    brightness = data["brightness"].as<byte>();
    setBrightness();

    isDirty = true;
  }

  if (data.containsKey("screenOnDistance"))
  {
    screenOnDistance = data["screenOnDistance"].as<byte>();

    isDirty = true;
  }

  if (data.containsKey("screenOnInterval"))
  {
    screenOnInterval = data["screenOnInterval"].as<unsigned long>();

    isDirty = true;
  }

  if (isDirty)
  {
    saveSettings();
    request->send(200);
    return;
  }

  request->send(400); // bad request
}

void setupWebserver()
{
  // rewrites
  server.rewrite("/", indexPath);
  server.rewrite("/index.html", indexPath);
  server.rewrite("/favicon.ico", "/bed-room-clock-dashboard/favicon.ico");

  // tunnel the index.html request
  server.on(indexPath, HTTP_GET, [&](AsyncWebServerRequest *request)
            {
      ClientRequestTunnel tunnel;
      if (tunnel.open(externalBaseUrl, request->url())) {
          String result = tunnel.getString();
          request->send(200, "text/html", result);
      } else {
          request->send(tunnel.getHttpCode());
      } });

  server.on("/bed-room-clock-dashboard/*", HTTP_GET, [&](AsyncWebServerRequest *request)
            {
    String moved_url = externalBaseUrl+request->url();
    request->redirect(moved_url); });

  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        StringStream stream;
        auto size = serializeJson(getInfoJson(), stream);

        request->send(stream, "application/json", size); });

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, settingsFilename, "application/json", false); });

  server.on("/api/hard-reset", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              request->send(200);

              SPIFFS.format();
              delay(1000);
              ESP.restart(); });

  server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              request->send(200);

              delay(1000);
              ESP.restart(); });

  server.addHandler(new AsyncCallbackJsonWebHandler("/api/settings", onChangeSettings));

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

void setupMDns()
{
  if (!MDNS.begin("bedroom-clock"))
  {
    Serial.println("Error starting mDNS");
  }

  MDNS.addService("http", "tcp", 80);
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
    ssd1306.print("connect to WiFi and");

    ssd1306.setCursor(0, 10);
    ssd1306.print("setup the device");

    ssd1306.setCursor(0, 30);
    ssd1306.print(WiFiSettings.hostname);

    ssd1306.setCursor(0, 40);
    ssd1306.print("password:");
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
    ssd1306.print("connecting to WiFi...");

    ssd1306.setCursor(0, 10);
    ssd1306.print(WiFi.SSID());

    ssd1306.display();
  };
  WiFiSettings.onSuccess = []()
  {
    ssd1306.clearDisplay();
    ssd1306.setTextSize(1);

    ssd1306.setCursor(0, 0);
    ssd1306.print("WiFi connected");

    ssd1306.setCursor(0, 10);
    ssd1306.print(WiFi.SSID());

    ssd1306.display();
  };
  WiFiSettings.onFailure = []()
  {
    ssd1306.clearDisplay();
    ssd1306.setTextSize(1);

    ssd1306.setCursor(0, 0);
    ssd1306.print("WiFi error ");

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

  loadSettings();

  setupDisplay();
  setupMDns();
  setupWifiSettings();
  setupMDns();
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
  byte screenOnDistance = settings["screenOnDistance"].as<byte>();
  if (lastDistance > 0 && lastDistance <= screenOnDistance)
  {
    // turn on display
    lastScreenOn = millis();
  }

  unsigned long screenOnInterval = settings["screenOnInterval"].as<unsigned long>();
  bool isDisplayOff = millis() - lastScreenOn >= screenOnInterval;
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