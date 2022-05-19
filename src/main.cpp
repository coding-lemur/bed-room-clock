#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SPIFFS.h>

#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <Ultrasonic.h>
#include <DHT.h>
#include <time.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>

#include "config.h"

Ultrasonic ultrasonic(GPIO_NUM_5, GPIO_NUM_18);
Adafruit_SSD1306 ssd1306(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
DHT dht(DHT_PIN, DHT_TYPE);

// TODO move to config
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

bool isPortalActive = false;
float lastTemperature = -100;
float lastHumidity = -100;

unsigned long lastDisplayUpdate = 0;
unsigned long lastTemperatureUpdate = 0;

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

  ssd1306.clearDisplay();

  ssd1306.setTextColor(WHITE);
  ssd1306.setTextSize(1);

  ssd1306.setCursor(0, 0);
  ssd1306.print(">> bedroom clock <<");

  ssd1306.display();
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

    if (millis() - lastTemperatureUpdate >= 2000)
    {
      // wait a two seconds between measurements
      lastTemperature = dht.readTemperature();
      lastHumidity = dht.readHumidity();

      lastTemperatureUpdate = millis();
    }

    int distance = ultrasonic.read(); // in cm

    if (distance < 8)
    {
      // TODO turn on display
    }

    if (millis() - lastDisplayUpdate >= 500)
    {
      Serial.println("Miau 5");

      struct tm timeinfo;
      if (!getLocalTime(&timeinfo))
      {
        Serial.println("Failed to obtain time");
        return;
      }

      ssd1306.clearDisplay();
      ssd1306.setTextSize(2);

      // print time
      ssd1306.setCursor(0, 10);
      ssd1306.print(&timeinfo, "%H");
      ssd1306.print(":");
      ssd1306.print(&timeinfo, "%M");
      /*ssd1306.print(":");
      ssd1306.print(&timeinfo, "%S");*/

      // TODO show progressbar for seconds value
      int16_t pixel = (122 * timeinfo.tm_sec) / 60;
      ssd1306.fillRect(3, 17, pixel, 1, SSD1306_INVERSE);

      ssd1306.setTextSize(1);

      if (lastTemperature > -100)
      {
        // print temperature
        ssd1306.setCursor(0, 30);
        ssd1306.print(lastTemperature);
        ssd1306.print(" C");
      }

      ssd1306.setCursor(0, 40);
      ssd1306.print(distance);

      ssd1306.display();

      lastDisplayUpdate = millis();
    }
  }
}