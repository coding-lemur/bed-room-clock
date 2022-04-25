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

#include "config.h"

Ultrasonic ultrasonic1(GPIO_NUM_5, GPIO_NUM_18);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
DHTesp dht;

void pre(void)
{
  /*u8g2.setFont(u8g2_font_squirrel_tu);
  u8g2.setFontDirection(0);
  // u8g2.clear();
  u8g2.clearBuffer();

  u8g2.print("U8g2 Library");
  u8g2.setCursor(0, 1);*/
}

// Start ArduinoOTA via WiFiSettings with the same hostname and password
void setup_ota()
{
  ArduinoOTA.setHostname(WiFiSettings.hostname.c_str());
  ArduinoOTA.setPassword(WiFiSettings.password.c_str());
  ArduinoOTA.begin();
}

void SetupDisplay()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  delay(1000);

  display.clearDisplay();
}

void setup()
{
  Serial.begin(9600);
  SPIFFS.begin(true); // Will format on the first run after failing to mount
  Wire.begin();

  dht.setup(GPIO_NUM_4, DHTesp::DHT22);

  SetupDisplay();

  WiFiSettings.secure = true;
  WiFiSettings.hostname = "bedroom-clock-"; // will auto add device ID
  WiFiSettings.password = PASSWORD;

  // Set callbacks to start OTA when the portal is active
  WiFiSettings.onPortal = []()
  {
    setup_ota();
  };
  WiFiSettings.onPortalWaitLoop = []()
  {
    ArduinoOTA.handle();
  };

  // Use stored credentials to connect to your WiFi access point.
  // If no credentials are stored or if the access point is out of reach,
  // an access point will be started with a captive portal to configure WiFi.
  WiFiSettings.connect();

  Serial.print("Password: ");
  Serial.println(WiFiSettings.password);

  setup_ota(); // If you also want the OTA during regular execution
}

void loop()
{
  ArduinoOTA.handle(); // If you also want the OTA during regular execution

  pre();

  int distance = ultrasonic1.read(); // in cm

  if (distance < 8)
  {
    // TODO turn on display
    // u8g2.setPowerSave(false);
  }

  float temperature = dht.getTemperature();
  // float humidity = dht.getHumidity();

  delay(1000);
}