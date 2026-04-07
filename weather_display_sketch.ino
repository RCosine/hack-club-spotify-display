#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

#define TFT_CS 1
#define TFT_RST 2
#define TFT_DC 3
#define TFT_SCLK 4
#define TFT_MOSI 5
#define BTN_STATE 6
#define BTN_REFRESH 7

auto ssid = "WifiSSID";
auto password = "WifiPassword123@";
auto api = "api1234-0";
auto ntpServer = "pool.ntp.org";
auto city = "City,US";
auto gmtOffset_sec = -25200;

String screenState = "home";
int temperature;
String current_weather;
float wind_speed;
String current_aqi;
String current_date;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  Serial.begin(115200);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  Serial.println("Let's begin!");
  tft.fillScreen(ST77XX_BLACK);
// button setpup
  pinMode(BTN_STATE, INPUT_PULLUP);
  pinMode(BTN_REFRESH, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Welcome. Trying to connect...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    tft.println(".");
  }
  tft.println("We're connected!");
  configTime(gmtOffset_sec, 0, ntpServer);
  fetchWeather();
  drawScreen();
}

void loop() {
  if (digitalRead(BTN_STATE) == LOW) {
    if (screenState == "home") {
      screenState = "secondary";
    } else if (screenState == "secondary")
    {
      screenState = "home";
    }
    drawScreen();
    delay(300);
  }

  if (digitalRead(BTN_REFRESH) == LOW) { 
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.println("Getting the latest and greatest...");
    fetchWeather();
    drawScreen();
    delay(300);
  }
}

void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeStringBuff[50];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%a %b %e", &timeinfo); 
      current_date = String(timeStringBuff);
    }

    String weatherPath = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + String(api) + "&units=imperial";
    http.begin(weatherPath.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<2048> doc;
      deserializeJson(doc, payload);
      temperature = doc["main"]["temp"];
      wind_speed = doc["wind"]["speed"];
      const char* condition = doc["weather"][0]["main"];
      current_weather = String(condition);
      float lat = doc["coord"]["lat"];
      float lon = doc["coord"]["lon"];
      http.end();
      String aqiPath = "http://api.openweathermap.org/data/2.5/air_pollution?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + String(api);
      http.begin(aqiPath.c_str());
      int aqiResponseCode = http.GET();

      if (aqiResponseCode > 0) {
        String aqiPayload = http.getString();
        StaticJsonDocument<512> aqiDoc;
        deserializeJson(aqiDoc, aqiPayload);
        
        int aqiValue = aqiDoc["list"][0]["main"]["aqi"];

        if (aqiValue == 1) {
          current_aqi = "Good fresh air!";
        } else if (aqiValue == 2) {
          current_aqi = "Acceptable...";
        } else if (aqiValue == 3) {
          current_aqi = "Lowkey not that good...";
        } else if (aqiValue == 4) {
          current_aqi = "Stay inside buddy";
        } else if (aqiValue == 5) {
          current_aqi = "GGs, we're cooked broski.";
        }
      }
      http.end();
    } else {
      http.end();
    }
  }
}

void drawScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(157, 203, 186));
  tft.println("Welcome, today is: " + current_date + "\n");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  if (screenState == "home")
  {
    tft.print(String(temperature) + " F\n");
    tft.setTextSize(1);
    tft.print("Condition: " + current_weather);
  }
  else if (screenState == "secondary")
  {
    tft.setTextSize(1);
    tft.print("The wind speed is " + String(wind_speed) + " mph\n");
    tft.print("The air quality is " + current_aqi);
  }
}