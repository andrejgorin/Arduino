#include <Arduino.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>

// const char *MYSSID = SSID; // your network SSID (name)
// const char *MYPASS = PASS; // your network password
// WiFiClient client;       // initialize wifi

// API key for ThingSpeak
// const char *myWriteAPIKey = T_AUTH;
// unsigned long myChannelNumber = SECRET_CH_ID;

// DS18B20 plugged into GPIO13
#define ONE_WIRE_BUS 13

// initiate oneWire for DS18B20
OneWire oneWire(ONE_WIRE_BUS);
// Pass oneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);
// address of DS18B20 in bedroom
uint8_t sensorBedroom[8] = {0x28, 0x31, 0x94, 0x19, 0x51, 0x20, 0x01, 0x88};
// resolution of DS18B20
const byte resolution = 9;

// initiate rtc
RTC_DS3231 rtc;

// set the LCD number of columns and rows
const byte lcdColumns = 20;
const byte lcdRows = 4;
//initiate lcd
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// to convert int to name of day of week
char daysOfTheWeek[7][10] = {"Sunday",
                             "Monday",
                             "Tuesday",
                             "Wednesday",
                             "Thursday",
                             "Friday",
                             "Saturday"};

char city[] = "Ogre, LV"; // first row on LCD
char myTime[20];          // second row on LCD
char myTemp[20];          // fourth row on LCD

const unsigned int serialSpeed = 115200; // speed of serial connection

long timing1 = 0;              // for timePeriod
const int timePeriod = 500;    // how often check time
long timing2 = 0;              // for tempPeriod
const int tempPeriod = 5000;   // how often check temperature
long timing3 = 0;              // for tempPeriod
const int thingPeriod = 60000; // how often check temperature

// declare functions
void centerLCD(int row, char text[]);
int myTemperature(DeviceAddress deviceAddress);
//void myWiFi();
void myLCD();
//void myThingSpeak();

void setup()
{
  // initiate serial communication
  Serial.begin(serialSpeed);
  // initiate DS18B20
  sensors.begin();
  // set resolution of DS18B20 to minimum
  sensors.setResolution(sensorBedroom, resolution);
  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // initialize thingspeak
  //WiFi.mode(WIFI_STA);
  //ThingSpeak.begin(client);
}

void loop()
{
  //myWiFi();
  myLCD();
  //myThingSpeak();
}

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

int myTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  int y = round(tempC);
  return y;
}

/*
void myWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(MYSSID);
    while (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(MYSSID, MYPASS); // Connect to WPA/WPA2 network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }
}
*/

void myLCD()
{
  if (millis() - timing1 > timePeriod)
  {
    timing1 = millis();
    DateTime now = rtc.now();
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

/*
void myThingSpeak()
{
  if (millis() - timing3 > thingPeriod)
  {
    timing3 = millis();
    int data = myTemperature(sensorBedroom);
    // Write value to Field 1 of a ThingSpeak Channel
    int httpCode = ThingSpeak.writeField(myChannelNumber, 1, data, myWriteAPIKey);

    if (httpCode == 200)
    {
      Serial.println("Channel write successful.");
    }
    else
    {
      Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
    }
  }
}
*/