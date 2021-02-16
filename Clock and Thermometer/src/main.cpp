/* Clock, calendar and thermometer.
 * Sends temperature data to ThingSpeak.
 * Based on ESP8266 chip.
 */

/***** libaries and files to include *****/

#include <Arduino.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <TaskScheduler.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <DST_RTC.h>
#include <ArduinoJson.h>
#include "MyCredentials.h"

/***** debug option ****/

// #define _DEBUG_
#ifdef _DEBUG_
#define serialSpeed 9600
#define SerialD Serial
#define _PM(a)             \
  SerialD.print(millis()); \
  SerialD.print(": ");     \
  SerialD.println(a)
#define _PP(a) SerialD.print(a)
#define _PL(a) SerialD.println(a)
#define _PX(a) SerialD.println(a, HEX)
#else
#define _PM(a)
#define _PP(a)
#define _PL(a)
#define _PX(a)
#endif

/***** OpenWeatherMap part *****/

String myLine = "";
int outTemp = 0;
int pressure = 0;
int outHumidity = 0;

/***** DST part *****/

DST_RTC dst_rtc;
const char rulesDST[] = "EU";

/***** NTP part *****/

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 7200;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

/***** wifi stuff *****/

const char *MYSSID = SSID; // network SSID (name)
const char *MYPASS = PASS; // network password
WiFiClient client;         // initialize wifi
bool myWiFiIsOk = true;    // false if http code from ThingSpeak is not 200

/***** DS18B20 part *****/

const byte ONE_WIRE_BUS = 13;                                                // DS18B20 plugged into GPIO13
OneWire oneWire(ONE_WIRE_BUS);                                               // initiate oneWire for DS18B20
DallasTemperature sensors(&oneWire);                                         // Pass oneWire reference to Dallas Temperature
uint8_t sensorBedroom[8] = {0x28, 0x31, 0x94, 0x19, 0x51, 0x20, 0x01, 0x88}; // address of DS18B20 in bedroom

/***** initiate rtc *****/

RTC_DS3231 rtc;
DateTime mNow;

/***** LCD screen part *****/

const byte lcdColumns = 20;                       // set number of columns of the LCD
const byte lcdRows = 4;                           // set number of rows of the LCD
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); //initiate lcd
char myTime[21];                                  // second row on LCD

/***** declare functions in loop *****/

void myLCD();
void myLCDTimer();
void myThingSpeak();
void myNTPUpdate();
void myActivationCallback();
void myTimeCheck();
void myGetWeather();

/***** declare helper functions *****/

void centerLCD(byte row, char text[]);
int myTemperature(DeviceAddress deviceAddress);
void checkResponse(int code);
void printSpace(byte count);
void getTempFJson();

/***** Task Scheduler stuff *****/

Scheduler ts;
Task t0(100 * TASK_MILLISECOND, TASK_FOREVER, &myTimeCheck);
Task t1(TASK_SECOND, TASK_FOREVER, &myLCD);
Task t2(2 * TASK_MINUTE, TASK_FOREVER, &myLCDTimer);
Task t3(5 * TASK_MINUTE, TASK_FOREVER, &myThingSpeak);
Task t4(24 * TASK_HOUR, TASK_FOREVER, &myNTPUpdate);
Task t5(TASK_SECOND, TASK_FOREVER, &myActivationCallback);
Task t6(20 * TASK_MINUTE, TASK_FOREVER, &myGetWeather);

void setup()
{
/***** initiate serial communication *****/
#if defined(_DEBUG_)
  Serial.begin(serialSpeed);
  delay(2000);
  _PL("begin: setup()");
#endif

  /***** initiate DS18B20 *****/

  const byte resolution = 9; // resolution of DS18B20
  sensors.begin();
  sensors.setResolution(sensorBedroom, resolution); // set resolution of DS18B20 to minimum

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
  t0.enable();
  t1.enable();
  t2.enable();
  t5.enable();
}

void loop()
{
  ts.execute();
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

/* function to get temperature from DS18B20, round it and convert to integer */
int myTemperature(DeviceAddress deviceAddress)
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(deviceAddress);
  int intTemp = round(tempC);
  return intTemp;
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
  char monthNames[12][4] = {"Jan",
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
  char fault[] = "No WiFi!";
  char myHum[21];  // third row on LCD
  char myTemp[21]; // fourth row on LCD
  static bool secCol = true;
  char *dynPrint;
  if (!myWiFiIsOk)
  {
    centerLCD(0, fault);
  }
  else
  {
    centerLCD(0, (char *)"                    "); // TODO temp solution
  }
  if (secCol)
  {
    dynPrint = (char *)" ";
  }
  else
  {
    dynPrint = (char *)":";
  }
  secCol = !secCol;
  sprintf(myTime,
          "%02i%s%02i %s %i %s",
          mNow.hour(),
          dynPrint,
          mNow.minute(),
          daysOfTheWeek[mNow.dayOfTheWeek()],
          mNow.day(),
          monthNames[mNow.month()]);
  centerLCD(1, myTime);
  sprintf(myHum, "H: %i%%, P: %i\"Hg", outHumidity, pressure);
  centerLCD(2, myHum);
  sprintf(myTemp,
          "In: %iC, Out: %iC",
          myTemperature(sensorBedroom),
          outTemp);
  centerLCD(3, myTemp);
}

/* function to send data to ThingSpeak */
void myThingSpeak()
{
  const char *myWriteAPIKey = T_AUTH;                 // api key for ThingSpeak
  const unsigned long myChannelNumber = SECRET_CH_ID; // channel id for ThingSpeak
  int data = myTemperature(sensorBedroom);
  ThingSpeak.setField(1, data); // Write value to a ThingSpeak Channel Field1
  ThingSpeak.setField(2, outTemp);
  ThingSpeak.setField(3, pressure);
  ThingSpeak.setField(4, outHumidity);
  ThingSpeak.setStatus(String("Last updated: ") + String(myTime)); // Write status to a ThingSpeak Channel
  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
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
    //rtc.adjust(DateTime(year(t), month(t), day(t), hour(t), minute(t), second(t)));
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
  const byte lcdOn = 6;
  const byte lcdOff = 22;
  int myHour = mNow.hour();
  if (myHour == lcdOn && !lcdState) // TODO refactor me
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
  float tempPress;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, myLine);
  tempTemp = doc["main"]["temp"];
  outTemp = round(tempTemp);
  tempPress = doc["main"]["pressure"];
  _PL(tempPress);
  pressure = round(tempPress / float(1.333));
  _PL(pressure);
  outHumidity = doc["main"]["humidity"];
}
