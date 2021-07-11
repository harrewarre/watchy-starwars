#include <Watchy.h>

#include "images\sw_images_background.h"

#include "font\SfDistantGalaxy_0l3d16pt7b.h"
#include "font\SfDistantGalaxy_0l3d8pt7b.h"

#define BLACK GxEPD_BLACK
#define WHITE GxEPD_WHITE

#define WEATHER_API "http://api.openweathermap.org/data/2.5/weather?q="
#define WEATHER_API_KEY ""
#define WEATHER_LOCATION ""

RTC_DATA_ATTR int weatherTemp;
RTC_DATA_ATTR int weatherCode;

RTC_DATA_ATTR uint8_t activeDay;
RTC_DATA_ATTR uint8_t activeHour;

class SmbWatchFace : public Watchy
{
public:
  void drawWatchFace()
  {
    if (isNewDay())
    {
      configureTime();
      sensor.resetStepCounter();
    }

    if (isNewHour())
    {
      updateWeatherInfo();
    }

    drawBackground();
    drawTime();
    drawDate();
    drawBattery();
    drawWeatherInfo();
    drawSteps();
  }

private:
  void drawBackground()
  {
    display.drawBitmap(0, 0, sw_images_background, 200, 200, WHITE);
  }

  void drawTime()
  {
    display.setFont(&SfDistantGalaxy_0l3d16pt7b);
    display.setTextColor(WHITE);

    display.setCursor(8, 92);
    if (currentTime.Hour < 10)
    {
      display.print("0");
    }
    display.print(currentTime.Hour);
    display.print(":");
    if (currentTime.Minute < 10)
    {
      display.print("0");
    }
    display.println(currentTime.Minute);
  }

  void drawDate()
  {
    display.setFont(&SfDistantGalaxy_0l3d8pt7b);
    display.setTextColor(WHITE);

    display.setCursor(8, 16);

    String day = getWeekDay(currentTime.Wday);
    day.toUpperCase();

    display.print(day);

    display.setCursor(8, 32);
    display.print(currentTime.Day);
    display.print("-");
    display.print(currentTime.Month);
    display.print("-");
    display.print(currentTime.Year + 1970);
  }

  void drawBattery()
  {
    display.setFont(&SfDistantGalaxy_0l3d16pt7b);
    display.setTextColor(BLACK);

    int batteryLevelPercent = 0;
    float vBat = getBatteryVoltage();

    float maxVBat = 4.45;
    float minVBat = 3.2;

    if (vBat >= maxVBat)
    {
      batteryLevelPercent = 100;
    }
    else if (vBat <= minVBat)
    {
      batteryLevelPercent = 0;
    }
    else
    {
      batteryLevelPercent = round(((vBat - minVBat) * 100) / (maxVBat - minVBat));
    }

    String soc = String(batteryLevelPercent) + "%";

    int16_t x, y;
    uint16_t w, h;

    display.getTextBounds(soc, 1, 1, &x, &y, &w, &h);
    display.setCursor(194 - w, 126);

    display.print(soc);
  }

  void drawWeatherInfo()
  {
    display.setFont(&SfDistantGalaxy_0l3d8pt7b);
    display.setTextColor(BLACK);

    String weatherText = "-";

    if (weatherCode == 999)
    {
      weatherText = "RTC";
    }
    else if (weatherCode == 800)
    {
      weatherText = "Sunny";
    }
    else if (weatherCode > 800)
    {
      weatherText = "Cloudy";
    }
    else if (weatherCode >= 700)
    {
      weatherText = "Mist";
    }
    else if (weatherCode >= 600)
    {
      weatherText = "Snow";
    }
    else if (weatherCode >= 500)
    {
      weatherText = "Rain";
    }
    else if (weatherCode >= 300)
    {
      weatherText = "Drizzle";
    }
    else if (weatherCode >= 200)
    {
      weatherText = "Thunder";
    }
    else
    {
      weatherText = "(unknown)";
    }

    String weatherInfo = weatherText + " // " + String(weatherTemp) + "°C";
    weatherInfo.toUpperCase();

    int16_t x, y;
    uint16_t w, h;

    display.getTextBounds(weatherInfo, 1, 1, &x, &y, &w, &h);
    display.setCursor(194 - w, 198);

    display.print(weatherInfo);
  }

  void drawSteps()
  {
    display.setFont(&SfDistantGalaxy_0l3d8pt7b);
    display.setTextColor(BLACK);

    int stepCount = sensor.getCounter();

    String weatherInfo = String(stepCount) + " steps";
    weatherInfo.toUpperCase();

    int16_t x, y;
    uint16_t w, h;

    display.getTextBounds(weatherInfo, 1, 1, &x, &y, &w, &h);
    display.setCursor(194 - w, 184);

    display.print(weatherInfo);
  }

  void updateWeatherInfo()
  {
    if (connectWiFi())
    {

      HTTPClient http;
      http.setConnectTimeout(5000);

      String weatherQueryURL = String(WEATHER_API) + String(WEATHER_LOCATION) + "&units=metric&appid=" + String(WEATHER_API_KEY);

      http.begin(weatherQueryURL.c_str());
      int httpResponseCode = http.GET();

      if (httpResponseCode == 200)
      {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);

        weatherTemp = int(responseObject["main"]["temp"]);
        weatherCode = int(responseObject["weather"][0]["id"]);
      }

      http.end();
      WiFi.mode(WIFI_OFF);
      btStop();
    }
    else
    {
      // No connection
      weatherTemp = RTC.temperature() / 4; //celsius
      weatherCode = 999;
    }
  }

  void configureTime()
  {
    if (connectWiFi())
    {
      int gmtOffset = 3600;      // +1hr.
      int daylightOffset = 3600; // Observe daylight savings.

      configTime(gmtOffset, daylightOffset, "pool.ntp.org");

      int i = 0;
      while (time(nullptr) < 1000000000l && i < 40)
      {
        delay(500);
        i++;
      }

      time_t tnow = time(nullptr);
      struct tm *local = localtime(&tnow);

      currentTime.Year = local->tm_year + 1900 - 18;
      currentTime.Month = local->tm_mon + 1;
      currentTime.Day = local->tm_mday;
      currentTime.Hour = local->tm_hour;
      currentTime.Minute = local->tm_min;
      currentTime.Second = local->tm_sec;
      currentTime.Wday = local->tm_wday + 1;
      RTC.write(currentTime);
      RTC.read(currentTime);
    }
  }

  bool isNewDay()
  {
    if (activeDay != currentTime.Day)
    {
      activeDay = currentTime.Day;
      return true;
    }

    return false;
  }

  bool isNewHour()
  {
    if (isNewDay() || activeHour != currentTime.Hour)
    {
      activeHour = currentTime.Hour;
      return true;
    }

    return false;
  }

  String getWeekDay(int dayNum)
  {
    switch (dayNum)
    {
    case 2:
      return "Monday";
    case 3:
      return "Tuesday";
    case 4:
      return "Wednesday";
    case 5:
      return "Thursday";
    case 6:
      return "Friday";
    case 7:
      return "Saturday";
    case 1:
      return "Sunday";
    default:
      return "(day ??)";
    }
  }
};

SmbWatchFace w;

void setup()
{
  w.init();
}

void loop() {}
