#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PZEM004Tv30.h>
#include "MyCredentials.h"

#define IOT_PUBLISH_TOPIC "esp32/pub"
#define IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

WiFiClient net;
PubSubClient client(net);

float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float frequency = 0;
float pf = 0;

void messageHandler(char *topic, byte *payload, unsigned int length);

void myPzem()
{ 
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();

  // Check if the data is valid
  if (isnan(voltage))
  {
    Serial.println("Error reading voltage");
  }
  else if (isnan(current))
  {
    Serial.println("Error reading current");
  }
  else if (isnan(power))
  {
    Serial.println("Error reading power");
  }
  else if (isnan(energy))
  {
    Serial.println("Error reading energy");
  }
  else if (isnan(frequency))
  {
    Serial.println("Error reading frequency");
  }
  else if (isnan(pf))
  {
    Serial.println("Error reading power factor");
  }
  else
  {

    // Print the values to the Serial console
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    Serial.print("Energy: ");
    Serial.print(energy);
    Serial.println("kWh");
    Serial.print("Frequency: ");
    Serial.print(frequency);
    Serial.println("Hz");
    Serial.print("PF: ");
    Serial.println(pf);
  }

  Serial.println();
  delay(2000);
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
}
void connectBroker()
{
  client.setServer(IOT_ENDPOINT, 1883);
  // Create a message handler
  client.setCallback(messageHandler);
  Serial.println("Connecting to broker");
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
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  if (!client.connected())
  {
    Serial.println("Broker timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(IOT_SUBSCRIBE_TOPIC);
}

void publishMessage()
{
  if (!client.connected())
  {
    connectBroker();
  }
  StaticJsonDocument<200> doc;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["frequency"] = frequency;
  doc["pf"] = pf;
  doc["did"] = WiFi.macAddress();
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
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

void setup()
{
  Serial.begin(9600);
  connectWiFi();
  connectBroker();
}

void loop()
{
  publishMessage();
  client.loop();
  delay(1000);
  myPzem();
}