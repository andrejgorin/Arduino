/* Testing some hardware and Blynk software */
/* TODO: temperature sensor ds18b20 */
/* TODO: i2c lcd screen */
/* TODO: RTC DS3231 */
/* TODO: ttl level converter TXS0108 */

/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial
// #define BLYNK_DEBUG

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <Bounce2.h>

/* where my sensitive info stored - MAIL, AUTH, SSID, PASS */
#include "MyCredentials.h"

/* section to add your sensitive settings */
const char *MYMAIL = MAIL;
const char *MYAUTH = AUTH;
const char *MYSSID = SSID;
const char *MYPASS = PASS;

/* virtual pins defined */
#define VPIN_UPTIME V5
#define VPIN_LED V2
#define VPIN_BUTTON V0
#define VPIN_LED2 V3
#define VPIN_BUTTON2 V1
#define VPIN_TIME V6
#define VPIN_GREEN V11
#define VPIN_GREENBUTTON V10

/* real pins defined */
#define RPIN_LED 2
#define RPIN_RELAY1 16
#define RPIN_GREENLED 5
#define RBUTTON_1 4

/* constants initialization */
const char SUBJECT[] = "Thermostat info";
const int SYNC_INTERVAL = (10 * 60);

/* variable initialization */
int ledValue = 255;
int ledValue2 = 255;
String fullip = "";
String body = "";
bool ledGreen = LOW;
bool blink = LOW;

/* blynk timer, button and RTC initialization */
BlynkTimer timer;
Button button = Button();
WidgetRTC rtc;

/* declaration of functions */
int myled(const bool value);
int toggleLed(bool state, int led, int vpin);
void mailme(const String body);
void myTimerEvent();

/* perform actions after connected to blynk server */
BLYNK_CONNECTED()
{
  Blynk.syncAll(); // sync all
  // Blynk.syncVirtual(V0); // FIXME sync only V0
  rtc.begin();
}

void setup()
{
  // Serial.begin(115200); // Debug console
  /* set modes and initial values for board pins */
  pinMode(RPIN_LED, OUTPUT);
  pinMode(RPIN_RELAY1, OUTPUT);
  pinMode(RPIN_GREENLED, OUTPUT);
  digitalWrite(RPIN_GREENLED, ledGreen);
  /* setup connection to blynk server */
  Blynk.begin(MYAUTH, MYSSID, MYPASS);
  /* get board ip address and send in email */
  IPAddress myip = WiFi.localIP();
  fullip = String(myip[0]) + "." + myip[1] + "." + myip[2] + "." + myip[3];
  body = "Thermostat started. IP address: " + fullip;
  mailme(body);
  /* define execution frequency of myTimerEvent */
  timer.setInterval(1000L, myTimerEvent);
  /* set sync interval for time synchronization */
  setSyncInterval(SYNC_INTERVAL);
  /* green button */
  button.attach(RBUTTON_1, INPUT);
  button.interval(50);
  button.setPressedState(HIGH);
}

void loop()
{
  /* begin Blynk and Blynk timer */
  Blynk.run();
  timer.run();
  /* handle button pressing */
  button.update();
  if (button.pressed())
  {
    ledGreen = !ledGreen;
    Blynk.virtualWrite(VPIN_GREENBUTTON, ledGreen);
  }
}

/* send email */
void mailme(const String body)
{
  Blynk.email(MYMAIL, SUBJECT, body);
}

/* push board uptime to blynk */
BLYNK_READ(VPIN_UPTIME)
{
  Blynk.virtualWrite(VPIN_UPTIME, millis() / 1000); // Arduino's uptime, in seconds
}

/* push board time to blynk */
BLYNK_READ(VPIN_TIME)
{
  String time = String(hour()) + ":" + minute() + ":" + second();
  String date = String(day()) + "-" + month() + "-" + year();
  Blynk.virtualWrite(VPIN_TIME, date + " " + time); // Arduino's current time
}

/* built in led */
BLYNK_WRITE(VPIN_BUTTON)
{
  int pinValue = param.asInt();
  digitalWrite(RPIN_LED, pinValue);
  ledValue = myled(pinValue);
}

/* relay red */
BLYNK_WRITE(VPIN_BUTTON2)
{
  int pinValue = param.asInt();
  digitalWrite(RPIN_RELAY1, pinValue);
  ledValue2 = myled(pinValue);
}

/* green button */
BLYNK_WRITE(VPIN_GREENBUTTON)
{
  int pinValue = param.asInt();
  if (pinValue != ledGreen)
  {
    ledGreen = !ledGreen;
  }
}

/* convert HIGH to 0 and LOW to 255 for leds */
int myled(const bool value)
{
  int ledvalue;
  if (value == HIGH)
  {
    ledvalue = 0;
  }
  else
  {
    ledvalue = 255;
  }
  return ledvalue;
}

/* push led data to blynk */
/* Please don't send more that 10 values per second */
void myTimerEvent()
{
  Blynk.virtualWrite(VPIN_LED, ledValue);
  Blynk.virtualWrite(VPIN_LED2, ledValue2);
  if (ledGreen == HIGH)
  {
    blink = toggleLed(blink, RPIN_GREENLED, VPIN_GREEN); // XXX just test
  }
  else
  {
    blink = toggleLed(HIGH, RPIN_GREENLED, VPIN_GREEN); // BUG just test
  }
}

/* toggle led state */
int toggleLed(bool state, int led, int vpin)
{
  state = state ? LOW : HIGH; // HACK just test
  digitalWrite(led, state);
  Blynk.virtualWrite(vpin, myled(!state));
  return state;
}

/* TODO cpp struct for all related variables */
