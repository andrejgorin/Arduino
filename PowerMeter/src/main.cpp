/* Power meter.
 * Sends data to MQTT broker.
 * Based on ESP32 chip.
 */

/***** libaries and files to include *****/
#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <TaskScheduler.h>
#include "MyCredentials.h"

/***** MQTT topics *****/
#define IOT_PUBLISH_TOPIC "esp32/pub"
#define IOT_SUBSCRIBE_TOPIC "esp32/sub"

/***** PZEM part *****/
#define DID "PowerMeter"
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2
#define ADDRESS 0x02
#define PHASE "2";
PZEM004Tv30 pzem2(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, ADDRESS);
int voltage = 0;
float current = 0;
int power = 0;
float energy = 0;
int frequency = 0;
float pf = 0;

/***** WiFi part *****/
WiFiClient net;

/***** MQTT part *****/
PubSubClient client(net);

/***** declare functions in loop *****/
void myPzem();
void checkConnections();

/***** declare helper functions *****/
void messageHandler(char *topic, byte *payload, unsigned int length);
void connectWiFi();
void connectBroker();
void publishMessage();

/***** Task Scheduler stuff *****/
Scheduler ts;
Task t0(5 * TASK_SECOND, TASK_FOREVER, &myPzem);
Task t1(15 * TASK_SECOND, TASK_FOREVER, &checkConnections);

void setup()
{
  Serial.begin(9600);
  connectWiFi();
  connectBroker();
  ts.addTask(t0);
  ts.addTask(t1);
  t0.enable();
  t1.enable();
}

void loop()
{
  client.loop();
  ts.execute();
}

void myPzem()
{
  voltage = round(pzem2.voltage());
  current = pzem2.current();
  power = round(pzem2.power());
  energy = pzem2.energy();
  frequency = round(pzem2.frequency());
  pf = pzem2.pf();
  publishMessage();
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

void connectBroker()
{
  client.setServer(IOT_ENDPOINT, 1883);
  client.setCallback(messageHandler);
  while (!client.connected())
  {
    if (client.connect(THINGNAME, MQTT_USER, MQTT_PASS))
    {
      Serial.println("Connected.");
    }
    else
    {
      Serial.print("Failed. Error state=");
      Serial.print(client.state());
      delay(500);
    }
  }
  client.subscribe(IOT_SUBSCRIBE_TOPIC);
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["frequency"] = frequency;
  doc["pf"] = pf;
  doc["did"] = DID;
  doc["phase"] = PHASE;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(char *topic, byte *payload, unsigned int length)
{ /***** test message: "{"message": "Hello, world!"}" *****/
  Serial.print("incoming topic: ");
  Serial.println(topic);
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload, length);
  const char *message = doc["message"];
  Serial.print("incoming message: ");
  Serial.println(message);
}

void checkConnections()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }
  if (!client.connected())
  {
    connectBroker();
  }
}