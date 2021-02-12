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
#include "MyCredentials.h"

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

/***** ThingSpeak stuff *****/

const char *myWriteAPIKey = T_AUTH;                 // api key for ThingSpeak
const unsigned long myChannelNumber = SECRET_CH_ID; // channel id for ThingSpeak

/***** DS18B20 part *****/

const byte ONE_WIRE_BUS = 13;                                                // DS18B20 plugged into GPIO13
OneWire oneWire(ONE_WIRE_BUS);                                               // initiate oneWire for DS18B20
DallasTemperature sensors(&oneWire);                                         // Pass oneWire reference to Dallas Temperature
uint8_t sensorBedroom[8] = {0x28, 0x31, 0x94, 0x19, 0x51, 0x20, 0x01, 0x88}; // address of DS18B20 in bedroom
const byte resolution = 9;                                                   // resolution of DS18B20

/***** initiate rtc *****/

RTC_DS3231 rtc;
DateTime mNow;

/***** LCD screen part *****/

const byte lcdColumns = 20;                       // set number of columns of the LCD
const byte lcdRows = 4;                           // set number of rows of the LCD
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); //initiate lcd
const byte lcdOn = 6;
const byte lcdOff = 22;
bool lcdState = true;
char daysOfTheWeek[7][10] = {"Sunday",
                             "Monday",
                             "Tuesday",
                             "Wednesday",
                             "Thursday",
                             "Friday",
                             "Saturday"}; // to convert int to name of day of week
char city[] = "Ogre, LV";                 // first row on LCD
char fault[] = "No WiFi!";
char myTime[20]; // second row on LCD
char myTemp[20]; // fourth row on LCD

/***** speed of serial connection *****/

const unsigned int serialSpeed = 9600;

/***** declare functions in loop *****/

void myLCD();
void myLCDTimer();
void myThingSpeak();
void myNTPUpdate();
void myActivationCallback();

/***** declare helper functions *****/

void centerLCD(int row, char text[]);
int myTemperature(DeviceAddress deviceAddress);
void checkResponse(int code);

/***** Task Scheduler stuff *****/

Scheduler ts;
Task t1(TASK_SECOND, TASK_FOREVER, &myLCD);
Task t2(2 * TASK_MINUTE, TASK_FOREVER, &myLCDTimer);
Task t3(5 * TASK_MINUTE, TASK_FOREVER, &myThingSpeak);
Task t4(24 * TASK_HOUR, TASK_FOREVER, &myNTPUpdate);
Task t5(TASK_SECOND, TASK_FOREVER, &myActivationCallback);

void setup()
{
  /***** initiate serial communication *****/

  Serial.begin(serialSpeed); // TODO activate Serial only in debug mode

  /***** initiate DS18B20 *****/

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

  ts.addTask(t1);
  ts.addTask(t2);
  ts.addTask(t3);
  ts.addTask(t4);
  ts.addTask(t5);
  t1.enable();
  t2.enable();
  t5.enable();
}

void loop()
{
  mNow = rtc.now(); // TODO not to check time every loop
  ts.execute();
}

/* function to place text in center of LCD row */
void centerLCD(int row, char text[])
{
  int prefix;
  int postfix;
  const char space[] = " ";
  lcd.setCursor(0, row);
  prefix = (lcdColumns - strlen(text)) / 2;
  for (int i = 0; i < prefix; i++) // TODO duplicate code, refactor to function
  {
    lcd.print(space);
  }
  lcd.print(text);
  postfix = lcdColumns - prefix - strlen(text);
  for (int i = 0; i < postfix; i++)
  {
    lcd.print(space);
  }
}

/* function to get temperature from DS18B20, round it and convert to integer */
int myTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  int intTemp = round(tempC);
  return intTemp;
}

/* function to print all necessary info on LCD */
void myLCD()
{
  if (myWiFiIsOk)
  {
    centerLCD(0, city);
  }
  else
  {
    centerLCD(0, fault);
  }
  sprintf(myTime,
          "%i/%02i/%02i %02i:%02i:%02i",
          mNow.year(),
          mNow.month(),
          mNow.day(),
          mNow.hour(),
          mNow.minute(),
          mNow.second());
  centerLCD(1, myTime);
  centerLCD(2, daysOfTheWeek[mNow.dayOfTheWeek()]);
  sensors.requestTemperatures();
  sprintf(myTemp, "Temp: %i C", myTemperature(sensorBedroom));
  centerLCD(3, myTemp);
}

/* function to send data to ThingSpeak */
void myThingSpeak()
{
  int data = myTemperature(sensorBedroom);
  ThingSpeak.setField(1, data);                                    // Write value to a ThingSpeak Channel Field1
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
    rtc.adjust(DateTime(year(t), month(t), day(t), hour(t), minute(t), second(t)));
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
  if (t5.getRunCounter() == 20)
  {
    t3.enable();
  }
  if (t5.getRunCounter() == 30)
  {
    t4.enable();
  }
}
