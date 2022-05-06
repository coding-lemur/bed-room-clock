#include <Arduino.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <Ultrasonic.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <DHTesp.h>
#include <time.h>

#include "config.h"

Ultrasonic ultrasonic1(GPIO_NUM_5, GPIO_NUM_18);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
DHTesp dht;

// TODO move to config
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

bool isPortalActive = false;
unsigned long lastDisplayUpdate = 0;

void setupDht()
{
  dht.setup(GPIO_NUM_4, DHTesp::DHT22);
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
  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  delay(1000);

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);

  display.setCursor(0, 0);
  display.print(">> bedroom clock <<");

  display.display();
}

void setupWifiSettings()
{
  WiFiSettings.secure = true;
  WiFiSettings.hostname = "bedroom-"; // will auto add device ID
  WiFiSettings.password = PASSWORD;

  // Set callbacks to start OTA when the portal is active
  WiFiSettings.onPortal = []()
  {
    isPortalActive = true;

    Serial.println("WiFi config portal active");

    setupOta();
  };
  WiFiSettings.onPortalWaitLoop = []()
  {
    ArduinoOTA.handle();
  };
  WiFiSettings.onConfigSaved = []()
  {
    ESP.restart();
  };

  // Use stored credentials to connect to your WiFi access point.
  // If no credentials are stored or if the access point is out of reach,
  // an access point will be started with a captive portal to configure WiFi.
  WiFiSettings.connect(true, 30);
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  SPIFFS.begin(true); // Will format on the first run after failing to mount
  // SPIFFS.format();    // TODO reset config on connection MQTT fail

  setupWifiSettings();
  setupDisplay();
  setupOta();
  setupNtp();
  setupDht();
}

void loop()
{
  ArduinoOTA.handle(); // If you also want the OTA during regular execution

  if (!isPortalActive)
  {
    Serial.println("Miau 1");

    int distance = ultrasonic1.read(); // in cm

    if (distance < 8)
    {
      // TODO turn on display
      // u8g2.setPowerSave(false);
    }

    if (millis() - lastDisplayUpdate >= 500)
    {
      Serial.println("Miau 5");

      float temperature = dht.getTemperature();
      // float humidity = dht.getHumidity();

      struct tm timeinfo;
      if (!getLocalTime(&timeinfo))
      {
        Serial.println("Failed to obtain time");
        return;
      }

      display.clearDisplay();
      display.setTextSize(3);

      display.setCursor(0, 0);
      display.print(temperature);
      display.print("°C");

      display.setCursor(3, 0);
      display.print(distance);

      display.display();

      lastDisplayUpdate = millis();
    }
  }

  // delay(1000);
}