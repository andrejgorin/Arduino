/* Clock, calendar and thermometer.
 * Sends temperature data to ThingSpeak.
 * Based on ESP8266 chip.
 */

/* libaries and files to include */
#include <Arduino.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include "MyCredentials.h"

/* wifi stuff */
const char *MYSSID = SSID; // network SSID (name)
const char *MYPASS = PASS; // network password
WiFiClient client;         // initialize wifi

/* ThingSpeak stuff */
const char *myWriteAPIKey = T_AUTH;                 // api key for ThingSpeak
const unsigned long myChannelNumber = SECRET_CH_ID; // channel id for ThingSpeak

/* DS18B20 part */
#define ONE_WIRE_BUS 13                                                      // DS18B20 plugged into GPIO13
OneWire oneWire(ONE_WIRE_BUS);                                               // initiate oneWire for DS18B20
DallasTemperature sensors(&oneWire);                                         // Pass oneWire reference to Dallas Temperature
uint8_t sensorBedroom[8] = {0x28, 0x31, 0x94, 0x19, 0x51, 0x20, 0x01, 0x88}; // address of DS18B20 in bedroom
const byte resolution = 9;                                                   // resolution of DS18B20

/* initiate rtc */
RTC_DS3231 rtc;
DateTime now;

/* LCD screen part */
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
char myTime[20];                          // second row on LCD
char myTemp[20];                          // fourth row on LCD

/* speed of serial connection */
const unsigned int serialSpeed = 9600; 

long timing1 = 0;              // for timePeriod
const int timePeriod = 500;    // how often check time
long timing2 = 0;              // for tempPeriod
const int tempPeriod = 5000;   // how often check temperature
long timing3 = 0;              // for thingPeriod
const int thingPeriod = 60000; // how often send data to ThingSpeak

/* declare functions */
void centerLCD(int row, char text[]);
int myTemperature(DeviceAddress deviceAddress);
void myWiFi();
void myLCD();
void myThingSpeak();
void checkResponse(int code);
void myLCDTimer();

void setup()
{
  /* initiate serial communication */
  Serial.begin(serialSpeed);

  /* initiate DS18B20 */
  sensors.begin();
  sensors.setResolution(sensorBedroom, resolution); // set resolution of DS18B20 to minimum

  /* initiate LCD */
  lcd.init();
  lcd.backlight(); // turn on LCD backlight
  //lcd.noBacklight();

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  /* Perform initial checking of RTC */
  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    /* When time needs to be set on a new device, or after a power loss, the
     * following line sets the RTC to the date & time this sketch was compiled
     */
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    /* This line sets the RTC with an explicit date & time, for example to set
     * January 21, 2014 at 3am you would call:
     */
    /* rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); */
  }
  /* When time needs to be re-set on a previously configured device, the
   * following line sets the RTC to the date & time this sketch was compiled
   */
  /* rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); */
  /* This line sets the RTC with an explicit date & time, for example to set
   * January 21, 2014 at 3am you would call:
   */
  /* rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); */

  /* initiate WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, MYPASS);

  /* initialize thingspeak */
  ThingSpeak.begin(client);
}

void loop()
{
  now = rtc.now();
  myLCD();
  // myWiFi();
  myThingSpeak();
  myLCDTimer();
}

/* function to place text in center of LCD row */
void centerLCD(int row, char text[])
{
  int prefix;
  int postfix;
  const char space[] = " ";
  lcd.setCursor(0, row);
  prefix = (lcdColumns - strlen(text)) / 2;
  for (int i = 0; i < prefix; i++)
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

/* function to check if WiFi is connected and try to connect if not */
// TODO refactor this function as it isn't usable now
void myWiFi()
{
  if (WiFi.isConnected() != true) // BUG WiFi.isConnected() doesn't work
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(MYSSID);
    while (WiFi.isConnected() != true) // BUG WiFi.isConnected() doesn't work
    {
      WiFi.begin(MYSSID, MYPASS); // Connect to WPA/WPA2 network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }
}

/* function to print all necessary info on LCD */
void myLCD()
{
  if (millis() - timing1 > timePeriod)
  {
    timing1 = millis();
    centerLCD(0, city);
    sprintf(myTime,
            "%i/%02i/%02i %02i:%02i:%02i",
            now.year(),
            now.month(),
            now.day(),
            now.hour(),
            now.minute(),
            now.second());
    centerLCD(1, myTime);
    centerLCD(2, daysOfTheWeek[now.dayOfTheWeek()]);
    if (millis() - timing2 > tempPeriod)
    {
      timing2 = millis();
      sensors.requestTemperatures();
    }
    sprintf(myTemp, "Temp: %i C", myTemperature(sensorBedroom));
    centerLCD(3, myTemp);
  }
}

/* function to send data to ThingSpeak */
void myThingSpeak()
{
  if (millis() - timing3 > thingPeriod)
  {
    timing3 = millis();
    int data = myTemperature(sensorBedroom);
    ThingSpeak.setField(1, data);                                    // Write value to a ThingSpeak Channel Field1
    ThingSpeak.setStatus(String("Last updated: ") + String(myTime)); // Write status to a ThingSpeak Channel
    int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    checkResponse(httpCode);
  }
}

/* function to check http response code from ThingSpeak. OK if 200, NOK in any other case */
void checkResponse(int code)
{
  if (code == 200)
  {
    Serial.println("Channel write successful.");
  }
  else
  {
    Serial.println("Problem writing to channel. HTTP error code " + String(code));
    WiFi.begin(MYSSID, MYPASS);
  }
}

/* function to turn lcd backlight on or off by timer */
void myLCDTimer()
{
  const byte lcdOn = 6;
  const byte lcdOff = 22;
  int myHour = now.hour();
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
