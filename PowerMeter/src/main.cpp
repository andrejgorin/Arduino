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
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "MyCredentials.h"

/***** Define devise name for Mdns and DB *****/

#define DID "PowerMeter"

/***** MQTT topics *****/

#define IOT_PUBLISH_TOPIC "esp32/pub"
#define IOT_SUBSCRIBE_TOPIC "esp32/sub"

/***** PZEM part *****/

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2
#define ADDRESS2 0x02
PZEM004Tv30 pzem2(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, ADDRESS2);
struct Measurement
{
  int voltage;
  float current;
  int power;
  float energy;
  int frequency;
  float pf;
  char phase[2];
};

/***** WiFi part *****/

WiFiClient net;

/***** MQTT part *****/

PubSubClient client(net);

/***** declare functions in loop *****/

void myPzem();
void checkConnections();

/***** declare helper functions *****/

void messageHandler(char *topic, byte *payload, unsigned int length); // not in use now
void readData(Measurement m, PZEM004Tv30 p, char ph[]);
void connectWiFi();
void connectBroker();
void publishMessage(Measurement);
void setupOTA();

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
  setupOTA();
}

void loop()
{
  ts.execute();
  client.loop();
  ArduinoOTA.handle();
}

/***** Read data from PZEM in TS loop *****/
void myPzem()
{
  Measurement msrm2;
  char tempP[2] = "2";
  readData(msrm2, pzem2, tempP);
}

/***** Read data from PZEM and send to broker *****/
void readData(Measurement m, PZEM004Tv30 p, char ph[])
{
  m.voltage = round(p.voltage());
  m.current = p.current();
  m.power = round(p.power());
  m.energy = p.energy();
  m.frequency = round(p.frequency());
  m.pf = p.pf();
  strcpy(m.phase, ph);
  publishMessage(m);
}

/***** Connect to WiFi *****/
void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

/***** Connect to broker *****/
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

/***** publish message *****/
void publishMessage(Measurement m)
{
  StaticJsonDocument<200> doc;
  doc["voltage"] = m.voltage;
  doc["current"] = m.current;
  doc["power"] = m.power;
  doc["energy"] = m.energy;
  doc["frequency"] = m.frequency;
  doc["pf"] = m.pf;
  doc["did"] = DID;
  doc["phase"] = m.phase;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(IOT_PUBLISH_TOPIC, jsonBuffer);
}

/***** callback (not in use now) *****/
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

/***** check connection to WiFi and broker in TS loop *****/
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

/***** Setup OTA *****/
void setupOTA()
{
  ArduinoOTA.setHostname(DID);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
  ArduinoOTA.begin();
}