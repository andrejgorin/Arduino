/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial
// #define BLYNK_DEBUG

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

#define MAIL "andrejgorin@gmail.com"
#define SUBJECT "Thermostat info"
#define SYNC_INTERVAL (10 * 60)
#define VPIN_UPTIME V5
#define VPIN_LED V2
#define RPIN_LED 2
#define VPIN_TIME V6
#define VPIN_BUTTON V0

char auth[] = "";
char ssid[] = "";
char pass[] = "";

int ledValue = 255;
String fullip;
String body;

BlynkTimer timer;
WidgetRTC rtc;

BLYNK_CONNECTED()
{
  Blynk.syncAll();       // sync all
  Blynk.syncVirtual(V0); // sync V0
  rtc.begin();
}

void setup()
{
  // Serial.begin(115200); // Debug console
  pinMode(RPIN_LED, OUTPUT);
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);
  IPAddress myip = WiFi.localIP();
  fullip = String(myip[0]) + "." + myip[1] + "." + myip[2] + "." + myip[3];
  body = "Thermostat started. IP address: " + fullip;
  mailme(body);
  // Blynk.email(MAIL, SUBJECT, body);
  setSyncInterval(SYNC_INTERVAL);
  // Serial.print("hour: ");
}

void loop()
{
  Blynk.run();
  timer.run();
}

void mailme(String body)
{
  Blynk.email(MAIL, SUBJECT, body);
}

BLYNK_READ(VPIN_UPTIME)
{
  Blynk.virtualWrite(VPIN_UPTIME, millis() / 1000); // Arduino's uptime
  String time = String(hour()) + ":" + minute() + ":" + second();
  String date = String(day()) + "-" + month() + "-" + year();
  Blynk.virtualWrite(VPIN_TIME, date + " " + time); // Arduino's current time
}

BLYNK_WRITE(VPIN_BUTTON)
{
  int pinValue = param.asInt();
  digitalWrite(RPIN_LED, pinValue);
  if (pinValue == 1)
  {
    ledValue = 0;
  }
  else
  {
    ledValue = 255;
  }
}

void myTimerEvent()
{
  Blynk.virtualWrite(VPIN_LED, ledValue); // Please don't send more that 10 values per second.
}
