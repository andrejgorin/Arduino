/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial
// #define BLYNK_DEBUG

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

// where my sensitive info stored - MAIL, AUTH, SSID, PASS
#include <MyCredentials.h>

// section to add your sensitive settings 
const char *MYMAIL = MAIL;
const char *MYAUTH = AUTH;
const char *MYSSID = SSID;
const char *MYPASS = PASS;

// virtual pins defined
#define VPIN_UPTIME V5
#define VPIN_LED V2
#define VPIN_LED2 V3
#define VPIN_TIME V6
#define VPIN_BUTTON V0
#define VPIN_BUTTON2 V1

// real pins defined
#define RPIN_LED 2
#define RPIN_RELAY1 16

// constants initialization
const char SUBJECT[] = "Thermostat info";
const int SYNC_INTERVAL = (10 * 60);

// variable initialization
int ledValue = 255;
int ledValue2 = 255;
String fullip = "";
String body = "";

// blynk timer and RTC initialization
BlynkTimer timer;
WidgetRTC rtc;

// declaration of functions
void mailme(const String body);
int myled(const int value);
void myTimerEvent();

// perform actions after connected to blynk server
BLYNK_CONNECTED()
{
  Blynk.syncAll();       // sync all
  // Blynk.syncVirtual(V0); // sync V0
  rtc.begin();
}

void setup()
{
  // Serial.begin(115200); // Debug console
  pinMode(RPIN_LED, OUTPUT);
  pinMode(RPIN_RELAY1, OUTPUT);
  Blynk.begin(MYAUTH, MYSSID, MYPASS);
  timer.setInterval(1000L, myTimerEvent);
  IPAddress myip = WiFi.localIP();
  fullip = String(myip[0]) + "." + myip[1] + "." + myip[2] + "." + myip[3];
  body = "Thermostat started. IP address: " + fullip;
  mailme(body);
  setSyncInterval(SYNC_INTERVAL);
  // Serial.print("hour: ");
}

void loop()
{
  Blynk.run();
  timer.run();
}

// send email
void mailme(const String body)
{
  Blynk.email(MYMAIL, SUBJECT, body);
}

// push board uptime to blynk
BLYNK_READ(VPIN_UPTIME)
{
  Blynk.virtualWrite(VPIN_UPTIME, millis() / 1000); // Arduino's uptime
}

// push board time to blynk
BLYNK_READ(VPIN_TIME)
{
  String time = String(hour()) + ":" + minute() + ":" + second();
  String date = String(day()) + "-" + month() + "-" + year();
  Blynk.virtualWrite(VPIN_TIME, date + " " + time); // Arduino's current time
}

// built in led
BLYNK_WRITE(VPIN_BUTTON)
{
  int pinValue = param.asInt();
  digitalWrite(RPIN_LED, pinValue);
  ledValue = myled(pinValue);
}

// relay 1
BLYNK_WRITE(VPIN_BUTTON2)
{
  int pinValue = param.asInt();
  digitalWrite(RPIN_RELAY1, pinValue);
  ledValue2 = myled(pinValue);
}

// convert 1 to 255 and 0 to 0 for leds
int myled(const int value)
{
  int ledvalue;
  if (value == 1)
  {
    ledvalue = 0;
  }
  else
  {
    ledvalue = 255;
  }
  return ledvalue;
}

// push led data to blynk
void myTimerEvent()
{
  Blynk.virtualWrite(VPIN_LED, ledValue); // Please don't send more that 10 values per second.
  Blynk.virtualWrite(VPIN_LED2, ledValue2);
}
