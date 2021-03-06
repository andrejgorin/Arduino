/* Testing some hardware and Blynk software */
/* TODO: temperature sensor ds18b20 */
/* TODO: i2c lcd screen */
/* TODO: RTC DS3231 */
/* TODO: ttl level converter TXS0108 */

/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial
// #define BLYNK_DEBUG

/* including public libraries */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h> 

/* where my sensitive info stored - MAIL, AUTH, SSID, PASS */
#include "MyCredentials.h"

/* section to add your sensitive settings */
const char *MYMAIL = MAIL;
const char *MYAUTH = AUTH;
const char *MYSSID = SSID;
const char *MYPASS = PASS;

/* declaration of functions */
/* check if WiFi connected and write result to real pin */
void isWifi();

/* example of some struct element */
struct element // TODO not used now
{
  bool state;
  int v_trigger;
  int r_trigger;
  int v_executor;
  int r_executor;
};

/* blue led on board. should be on when wifi connected */
struct blueLedOnBoard
{
  const int r_executor = 2;
};

blueLedOnBoard bLOB;

/* initializations */

/* Blynk timer */
BlynkTimer timer;

/* perform actions after connected to blynk server */
BLYNK_CONNECTED()
{
  Blynk.syncAll();
}

void setup()
{
  /* set modes and initial states for pins */
  pinMode(bLOB.r_executor, OUTPUT);
  digitalWrite(bLOB.r_executor, HIGH);

  /* setup connection to blynk server */
  Blynk.begin(MYAUTH, MYSSID, MYPASS);

  /* perform periodic check */
  timer.setInterval(1000, isWifi);
}

void loop()
{
  /* begin Blynk */
  Blynk.run();
  timer.run();
}

/* check if WiFi connected and write result to real pin */
void isWifi()
{
  digitalWrite(bLOB.r_executor, !Blynk.connected());
}