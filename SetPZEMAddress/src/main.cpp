/* Power meter.
 * Sends data to MQTT broker.
 * Based on ESP32 chip.
 */

/***** libaries and files to include *****/
#include <Arduino.h>
#include <PZEM004Tv30.h>

/***** PZEM part *****/
#define SET_ADDRESS 0x02 // Change to necessary address!!!
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  static uint8_t addr = SET_ADDRESS;
  Serial.print("Previous address:   0x");
  Serial.println(pzem.readAddress(), HEX);
  
  Serial.print("Setting address...");
  Serial.println(addr, HEX);
  if (!pzem.setAddress(addr))
  {
    Serial.println("Error setting address.");
  }
  else
  {
    // Print out the new custom address
    Serial.print("Current address:    0x");
    Serial.println(pzem.readAddress(), HEX);
    Serial.println();
  }
  delay(5000);
}