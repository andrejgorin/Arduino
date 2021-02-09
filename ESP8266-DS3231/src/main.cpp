#include <Arduino.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>

// DS18B20 plugged into GPIO13
#define ONE_WIRE_BUS 13

// set the LCD number of columns and rows
int lcdColumns = 20;
int lcdRows = 4;

// initiate oneWire
OneWire oneWire(ONE_WIRE_BUS);
// initiate rtc
RTC_DS3231 rtc;
//initiate lcd
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// variables
uint8_t sensorBedroom[8] = {0x28, 0x31, 0x94, 0x19, 0x51, 0x20, 0x01, 0x88};
char daysOfTheWeek[7][10] = {"Sunday",
                             "Monday",
                             "Tuesday",
                             "Wednesday",
                             "Thursday",
                             "Friday",
                             "Saturday"};
char myTemp[20];
char tempBuffer[8];
char city[] = "Ogre, LV";
char myTime[20];
unsigned long timing;

// declare functions
void centerLCD(int row, char text[]);
float myTemperature(DeviceAddress deviceAddress);

void setup()
{
  // initiate serial communication
  Serial.begin(115200);
  // initiate DS18B20
  sensors.begin();
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
}

void loop()
{
  if (millis() - timing > 500)
  {
  DateTime now = rtc.now();
  // lcd.clear();
  centerLCD(0, city);
  sprintf(myTime,
          "%i/%02i/%02i %02i:%02i",
          now.year(),
          now.month(),
          now.day(),
          now.hour(),
          now.minute());
  centerLCD(1, myTime);
  centerLCD(2, daysOfTheWeek[now.dayOfTheWeek()]);
  if (millis() - timing > 5000)
  {
  sensors.requestTemperatures();
  }
  dtostrf(myTemperature(sensorBedroom), 5, 2, tempBuffer);
  sprintf(myTemp, "Temp: %s C", tempBuffer);
  centerLCD(3, myTemp);
  }
}

void centerLCD(int row, char text[])
{
  int prefix;
  int postfix;
  lcd.setCursor(0, row);
  prefix = (lcdColumns - strlen(text)) / 2;
  for (int i = 0; i < prefix; i++)
  {
    lcd.print(" ");
  }
  lcd.print(text);
  postfix = lcdColumns - prefix - strlen(text);
  for (int i = 0; i < postfix; i++)
  {
    lcd.print(" ");
  }
}

float myTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  return tempC;
}
