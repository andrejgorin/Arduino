/* Clock, calendar, thermometer, humidity, pressure.
 * NTP, additional weather data from OpenWeatherMap.
 * Sends data to ThingSpeak.
 * Based on ESP8266 chip.
 */

/***** libaries and files to include *****/

#include <Arduino.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TaskScheduler.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <DST_RTC.h>
#include <ArduinoJson.h>
#include <MHZ19.h>
#include <SoftwareSerial.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "MyCredentials.h"

/***** debug option ****/

// #define _DEBUG_
#ifdef _DEBUG_
#define serialSpeed 9600
#define SerialD Serial
#define _PP(a) SerialD.print(a)
#define _PL(a) SerialD.println(a)
#else
#define _PP(a)
#define _PL(a)
#endif

/***** CO2 sensor *****/

const bool calibreMe = false; // if true, MH-Z19B will be calibrated in a 20 minutes
const byte RX_PIN = 12;       // D6 on ESP8266
const byte TX_PIN = 15;       // D8 on ESP8266
const unsigned int BAUDRATE = 9600;
int CO2 = 0;
MHZ19 myMHZ19;
SoftwareSerial mySerial(RX_PIN, TX_PIN);

/***** Pressure and temperature on BME280 *****/

Adafruit_BME280 bme;
int tf = 0;
int pf = 0;
int hf = 0;

/***** OpenWeatherMap part *****/

String myLine = "";
int outTemp = 0;
int outWind = 0;
int outGust = 0;
int outHumidity = 0;
int outDirection = 0;
char outDir[4];

/***** DST part *****/

DST_RTC dst_rtc;
const char rulesDST[] = "EU";

/***** NTP part *****/

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 7200;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

/***** wifi stuff *****/

#define DID "WeatherClock"
const char *MYSSID = SSID; // network SSID (name)
const char *MYPASS = PASS; // network password
WiFiClient client;         // initialize wifi
bool myWiFiIsOk = true;    // false if http code from ThingSpeak is not 200

/***** initiate rtc *****/

RTC_DS3231 rtc;
DateTime mNow;

/***** LCD screen part *****/

const byte lcdColumns = 20;                       // set number of columns of the LCD
const byte lcdRows = 4;                           // set number of rows of the LCD
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); // initiate lcd
char myFirst[21];                                 // second row on LCD

/***** declare functions in loop *****/

void myLCD();
void myLCDTimer();
void myThingSpeak();
void myNTPUpdate();
void myActivationCallback();
void myTimeCheck();
void myGetWeather();
void myGetBME280();
void myGetCO2();
void myCalibration();

/***** declare helper functions *****/

void centerLCD(byte row, char text[]);
void checkResponse(int code);
void printSpace(byte count);
void getTempFJson();
void getDirLit(int outDirection);
void setupOTA();

/***** Task Scheduler stuff *****/

Scheduler ts;
Task t0(100 * TASK_MILLISECOND, TASK_FOREVER, &myTimeCheck);
Task t1(TASK_SECOND, TASK_FOREVER, &myLCD);
Task t2(2 * TASK_MINUTE, TASK_FOREVER, &myLCDTimer);
Task t3(5 * TASK_MINUTE, TASK_FOREVER, &myThingSpeak);
Task t4(1 * TASK_HOUR, TASK_FOREVER, &myNTPUpdate);
Task t5(TASK_SECOND, TASK_FOREVER, &myActivationCallback);
Task t6(20 * TASK_MINUTE, TASK_FOREVER, &myGetWeather);
Task t7(5 * TASK_SECOND, TASK_FOREVER, &myGetBME280);
Task t8(20 * TASK_SECOND, TASK_FOREVER, &myGetCO2);
Task t9(TASK_ONCE, TASK_FOREVER, &myCalibration);

void setup()
{
/***** initiate serial communication *****/
#if defined(_DEBUG_)
  Serial.begin(serialSpeed);
  delay(2000);
  _PL("Debug mode begin: setup()");
#endif

  /***** initiate MH-Z19B *****/

  mySerial.begin(BAUDRATE);
  myMHZ19.begin(mySerial);
  myMHZ19.setFilter(true, true);
  myMHZ19.autoCalibration(false);

  /***** initiate BME280 *****/

  bme.begin(0x76); // address of sensor on i2c

  /***** initiate LCD *****/

  lcd.init();
  lcd.backlight(); // turn on LCD backlight

  /***** RTC part *****/

  rtc.begin();

  /***** initiate WiFi *****/

  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, MYPASS);

  /***** initialize thingspeak *****/

  ThingSpeak.begin(client);

  /***** initialize NTP *****/

  timeClient.begin();

  /***** add all tasks to TaskScheduler and enable *****/

  ts.addTask(t0);
  ts.addTask(t1);
  ts.addTask(t2);
  ts.addTask(t3);
  ts.addTask(t4);
  ts.addTask(t5);
  ts.addTask(t6);
  ts.addTask(t7);
  ts.addTask(t8);
  ts.addTask(t9);
  t0.enable();
  t1.enable();
  t2.enable();
  t5.enable();
  t7.enable();
  t8.enable();

  /***** begin OTA *****/

  setupOTA();
}

void loop()
{
  ts.execute();
  ArduinoOTA.handle();
}

/* function to calibrate MH-Z19B */
void myCalibration()
{
  myMHZ19.calibrate();
}

/* function to get data from MH-Z19B */
void myGetCO2()
{
  int tempCO2 = myMHZ19.getCO2(true);
  if (tempCO2 != 0)
  {
    CO2 = tempCO2;
  }
  _PP("CO2: ");
  _PL(CO2);
}

/* function to get data from BME280 */
void myGetBME280()
{
  tf = round(bme.readTemperature());
  pf = round(bme.readPressure() / float(133.3));
  hf = round(bme.readHumidity());
  _PL(tf);
  _PL(pf);
  _PL(hf);
}
/* just tiny peace of code to get current time from DS3231 */
void myTimeCheck()
{
  mNow = rtc.now();
}

/* function to place text in center of LCD row */
void centerLCD(int row, char text[])
{
  int prefix;
  int postfix;
  lcd.setCursor(0, row);
  prefix = (lcdColumns - strlen(text)) / 2;
  printSpace(prefix);
  lcd.print(text);
  postfix = lcdColumns - prefix - strlen(text);
  printSpace(postfix);
}

/* function to print " " as much as you need */
void printSpace(byte count)
{
  const char space[] = " ";
  for (int i = 0; i < count; i++)
  {
    lcd.print(space);
  }
}

/* function to print all necessary info on LCD */
void myLCD()
{
  char daysOfTheWeek[7][4] = {"Sun",
                              "Mon",
                              "Tue",
                              "Wed",
                              "Thu",
                              "Fri",
                              "Sat"}; // to convert int to name of day of week
  char monthNames[13][4] = {"",
                            "Jan",
                            "Feb",
                            "Mar",
                            "Apr",
                            "May",
                            "Jun",
                            "Jul",
                            "Aug",
                            "Sep",
                            "Oct",
                            "Nov",
                            "Dec"}; // to convert int to name of day of week
  char fault[] = "No WiFi!";        // alternative first row
  char mySecond[21];                // second row on LCD
  char myThird[21];                 // third row on LCD
  char myFourth[21];                // fourth row on LCD
  static bool secCol = true;
  char *dynPrint;
  if (secCol)
  {
    dynPrint = (char *)" ";
  }
  else
  {
    dynPrint = (char *)":";
  }
  sprintf(myFirst,
          "%02i%s%02i %s %i %s",
          mNow.hour(),
          dynPrint,
          mNow.minute(),
          daysOfTheWeek[mNow.dayOfTheWeek()],
          mNow.day(),
          monthNames[mNow.month()]); // alternative first row
  if (!myWiFiIsOk)
  {
    centerLCD(0, fault);
  }
  else
  {
    centerLCD(0, myFirst);
  }
  secCol = !secCol;
  sprintf(mySecond, "I:%iC,%i%%,%ippm", tf, hf, CO2);
  centerLCD(1, mySecond);
  sprintf(myThird, "O:%iC,%i%%,%i\"Hg,", outTemp, outHumidity, pf);
  centerLCD(2, myThird);
  sprintf(myFourth, "%s %i(%i)m/s", outDir, outWind, outGust);
  centerLCD(3, myFourth);
}

/* function to send data to ThingSpeak */
void myThingSpeak()
{
  const char *myWriteAPIKey = T_AUTH; // api key for ThingSpeak
  ThingSpeak.setField(1, outWind);
  ThingSpeak.setField(2, outTemp);
  ThingSpeak.setField(3, outGust);
  ThingSpeak.setField(4, outHumidity);
  ThingSpeak.setField(5, tf);
  ThingSpeak.setField(6, pf);
  ThingSpeak.setField(7, hf);
  if (CO2 != 0)
  {
    ThingSpeak.setField(8, CO2);
  }
  ThingSpeak.setStatus(String("Last updated: ") + String(myFirst)); // Write status to a ThingSpeak Channel
  int httpCode = ThingSpeak.writeFields(SECRET_CH_ID, myWriteAPIKey);
  checkResponse(httpCode);
}

/* function to get time by NTP and adjust RTC, taking DST in consideration */
void myNTPUpdate()
{
  if (timeClient.update())
  {
    unsigned long t = timeClient.getEpochTime();
    if (dst_rtc.checkDST(t) == true)
    {
      t = t + 3600;
    }
    rtc.adjust(t);
  }
}

/* function to check http response code from ThingSpeak. OK if 200, NOK in any other case */
void checkResponse(int code)
{
  if (code == 200)
  {
    myWiFiIsOk = true;
  }
  else
  {
    myWiFiIsOk = false;
  }
}

/* function to turn lcd backlight on or off by timer */
void myLCDTimer()
{
  static bool lcdState = true;
  const byte lcdOn = 7;
  const byte lcdOff = 21;
  int myHour = mNow.hour();
  if (myHour == lcdOn && !lcdState)
  {
    lcd.backlight();
    lcdState = !lcdState;
  }
  if (myHour == lcdOff && lcdState)
  {
    lcd.noBacklight();
    lcdState = !lcdState;
  }
}

/* function to activate some tasks with delay */
void myActivationCallback()
{
  if (t5.getRunCounter() == 25)
  {
    t3.enable();
  }
  if (t5.getRunCounter() == 20)
  {
    t6.enable();
  }
  if (t5.getRunCounter() == 30)
  {
    t4.enable();
  }
  if (t5.getRunCounter() == 1200 && calibreMe)
  {
    t9.enable();
  }
}

/* function to get current temperature from openweathermap */
void myGetWeather()
{
  const String cityID = "457065"; // Ogre
  const String oWMKey = OW_KEY;   // API key for OpenWeatherMap
  const char server[] = "api.openweathermap.org";
  if (client.connect(server, 80))
  {
    client.print("GET /data/2.5/weather?");
    client.print("id=" + cityID);
    client.print("&appid=" + oWMKey);
    client.println("&units=metric");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
    while (client.connected())
    {
      myLine = client.readStringUntil('\n');
      _PL(myLine);
      getTempFJson();
    }
  }
  else
  {
    _PL("unable to connect");
  }
}

void getTempFJson()
{
  float tempTemp;
  float tempWind;
  float tempGust;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, myLine);
  tempTemp = doc["main"]["temp"];
  outTemp = round(tempTemp);
  tempWind = doc["wind"]["speed"];
  outWind = round(tempWind);
  tempGust = doc["wind"]["gust"];
  outGust = round(tempGust);
  outHumidity = doc["main"]["humidity"];
  outDirection = doc["wind"]["deg"];
  getDirLit(outDirection);
}

void getDirLit(int outDirection)
{
  if (outDirection >= 349)
  {
    strcpy(outDir, "N");
  }
  if (outDirection <= 11)
  {
    strcpy(outDir, "N");
  }
  if ((outDirection >= 12) && (outDirection <= 33))
  {
    strcpy(outDir, "NNE");
  }
  if ((outDirection >= 34) && (outDirection <= 56))
  {
    strcpy(outDir, "NE");
  }
  if ((outDirection >= 57) && (outDirection <= 78))
  {
    strcpy(outDir, "ENE");
  }
  if ((outDirection >= 79) && (outDirection <= 101))
  {
    strcpy(outDir, "E");
  }
  if ((outDirection >= 102) && (outDirection <= 123))
  {
    strcpy(outDir, "ESE");
  }
  if ((outDirection >= 124) && (outDirection <= 146))
  {
    strcpy(outDir, "SE");
  }
  if ((outDirection >= 147) && (outDirection <= 168))
  {
    strcpy(outDir, "SSE");
  }
  if ((outDirection >= 169) && (outDirection <= 191))
  {
    strcpy(outDir, "S");
  }
  if ((outDirection >= 192) && (outDirection <= 213))
  {
    strcpy(outDir, "SSW");
  }
  if ((outDirection >= 214) && (outDirection <= 236))
  {
    strcpy(outDir, "SW");
  }
  if ((outDirection >= 237) && (outDirection <= 258))
  {
    strcpy(outDir, "WSW");
  }
  if ((outDirection >= 259) && (outDirection <= 281))
  {
    strcpy(outDir, "W");
  }
  if ((outDirection >= 282) && (outDirection <= 303))
  {
    strcpy(outDir, "WNW");
  }
  if ((outDirection >= 304) && (outDirection <= 326))
  {
    strcpy(outDir, "NW");
  }
  if ((outDirection >= 327) && (outDirection <= 348))
  {
    strcpy(outDir, "NNW");
  }
}

void setupOTA()
{
  ArduinoOTA.setHostname(DID);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
  ArduinoOTA.begin();
}