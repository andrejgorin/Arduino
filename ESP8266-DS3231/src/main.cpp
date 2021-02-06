#include <Arduino.h>
#include <Wire.h>      //I2C library
#include <RtcDS3231.h> //RTC library

RtcDS3231<TwoWire> rtcObject(Wire); // for version 2.0.0 of the rtc library

void setup()
{

  Serial.begin(115200); //Starts serial connection
  rtcObject.Begin();    //Starts I2C
  /* uncomment to setup time
  RtcDateTime currentTime = RtcDateTime(21, 02, 06, 16, 30, 0); //define date and time object
  rtcObject.SetDateTime(currentTime);                           //configure the RTC with object
  */
}

void loop()
{

  RtcDateTime currentTime = rtcObject.GetDateTime(); //get the time from the RTC

  char str[20]; //declare a string as an array of chars

  sprintf(str, "%d/%d/%d %d:%d:%d", //%d allows to print an integer to the string
          currentTime.Year(),       //get year method
          currentTime.Month(),      //get month method
          currentTime.Day(),        //get day method
          currentTime.Hour(),       //get hour method
          currentTime.Minute(),     //get minute method
          currentTime.Second()      //get second method
  );

  Serial.println(str); //print the string to the serial port

  delay(20000); //20 seconds delay
}