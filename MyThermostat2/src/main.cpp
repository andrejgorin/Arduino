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

struct element
{
  bool state;
  int vbutton;
  int rbutton;
  int vled;
  int rled;
};

/* perform actions after connected to blynk server */
BLYNK_CONNECTED()
{
  Blynk.syncAll();
}

void setup()
{
  /* setup connection to blynk server */
  Blynk.begin(MYAUTH, MYSSID, MYPASS);
}

void loop()
{
  /* begin Blynk */
  Blynk.run();
}