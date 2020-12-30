/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_DEBUG
#define DEBUG_PRINT

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = "";
char ssid[] = "";
char pass[] = "";
int ledValue = 255;
int LED = 2;

BlynkTimer timer;

void setup()
{
  // Debug console
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);
  Blynk.email("andrejgorin@gmail.com", "MySubject", "Thermostat started");
}

void loop()
{
  Blynk.run();
  timer.run();
}

BLYNK_WRITE(V0)
{
  int pinValue = param.asInt();
  digitalWrite(LED, pinValue);
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
  Blynk.virtualWrite(V2, ledValue); // Please don't send more that 10 values per second.
}

BLYNK_CONNECTED() { Blynk.syncAll(); }
