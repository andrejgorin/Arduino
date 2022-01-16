#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "MyCredentials.h"

#define IOT_PUBLISH_TOPIC "esp32/pub"
#define IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClient net;
PubSubClient client(net);

void messageHandler(char *topic, byte *payload, unsigned int length);

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
  client.setServer(IOT_ENDPOINT, 1880);
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
  int hum = random(60, 70);
  int temp = random(-20, -10);
  doc["humidity"] = hum;    // CHANGEME
  doc["temperature"] = temp; // CHANGEME
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
}