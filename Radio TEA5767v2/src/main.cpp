#include <Arduino.h>
#include <Wire.h>

#include <radio.h>
#include <TEA5767.h>
#include <RDSParser.h>


// Define some stations available at your locations here:
// 89.40 MHz as 8940

RADIO_FREQ preset[] = {
  10620,
  10570
};

int    i_sidx = 0;        ///< Start at Station with index=5

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;       ///< Create an instance of a non functional radio.
// RDA5807M radio;    ///< Create an instance of a RDA5807 chip radio
// SI4703   radio;    ///< Create an instance of a SI4703 chip radio.
//SI4705   radio;    ///< Create an instance of a SI4705 chip radio.
TEA5767  radio;    ///< Create an instance of a TEA5767 chip radio.


/// get a RDS parser
RDSParser rds;


/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_PARSECOMMAND, ///< waiting for a new command character.

  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC          ///< executing the command.
};

RADIO_STATE state; ///< The state variable is used for parsing input characters.

// - - - - - - - - - - - - - - - - - - - - - - - - - -



/// Update the Frequency on the LCD display.
void DisplayFrequency(RADIO_FREQ f)
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("FREQ:"); Serial.println(s);
} // DisplayFrequency()


/// Update the ServiceName text on the LCD display.
void DisplayServiceName(char *name)
{
  Serial.print("RDS:");
  Serial.println(name);
} // DisplayServiceName()


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runSerialCommand(char cmd, int16_t value)
{
  if (cmd == '?') {
    Serial.println();
    Serial.println("? Help");
    Serial.println("+ increase volume");
    Serial.println("- decrease volume");
    Serial.println("> next preset");
    Serial.println("< previous preset");
    Serial.println(". scan up   : scan up to next sender");
    Serial.println(", scan down ; scan down to next sender");
    Serial.println("fnnnnn: direct frequency input");
    Serial.println("i station status");
    Serial.println("s mono/stereo mode");
    Serial.println("b bass boost");
    Serial.println("u mute/unmute");
  }

  // ----- control the volume and audio output -----

  else if (cmd == '+') {
    // increase volume
    int v = radio.getVolume();
    if (v < 15) radio.setVolume(++v);
  } else if (cmd == '-') {
    // decrease volume
    int v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);
  }

  else if (cmd == 'u') {
    // toggle mute mode
    radio.setMute(!radio.getMute());
  }

  // toggle stereo mode
  else if (cmd == 's') { radio.setMono(!radio.getMono()); }

  // toggle bass boost
  else if (cmd == 'b') { radio.setBassBoost(!radio.getBassBoost()); }

  // ----- control the frequency -----

  else if (cmd == '>') {
    // next preset
    if (i_sidx < (sizeof(preset) / sizeof(RADIO_FREQ)) - 1) {
      i_sidx++; radio.setFrequency(preset[i_sidx]);
    } // if
  } else if (cmd == '<') {
    // previous preset
    if (i_sidx > 0) {
      i_sidx--;
      radio.setFrequency(preset[i_sidx]);
    } // if

  } else if (cmd == 'f') { radio.setFrequency(value); }

  else if (cmd == '.') { radio.seekUp(false); } else if (cmd == ':') { radio.seekUp(true); } else if (cmd == ',') { radio.seekDown(false); } else if (cmd == ';') { radio.seekDown(true); }


  // not in help:
  else if (cmd == '!') {
    if (value == 0) radio.term();
    if (value == 1) radio.init();

  } else if (cmd == 'i') {
    char s[12];
    radio.formatFrequency(s, sizeof(s));
    Serial.print("Station:"); Serial.println(s);
    Serial.print("Radio:"); radio.debugRadioInfo();
    Serial.print("Audio:"); radio.debugAudioInfo();

  } // info

  else if (cmd == 'x') {
    radio.debugStatus(); // print chip specific data.
  }
} // runSerialCommand()


/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  // open the Serial port
  Serial.begin(9600);
  Serial.print("Radio...");
  delay(500);

  // Initialize the Radio 
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, preset[i_sidx]); // 5. preset.

  // delay(100);

  radio.setMono(false);
  radio.setMute(false);
  // radio._wireDebug();
  radio.setVolume(8);

  Serial.write('>');

  state = STATE_PARSECOMMAND;

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayServiceName);

  runSerialCommand('?', 0);
} // Setup


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  int newPos;
  unsigned long now = millis();
  static unsigned long nextFreqTime = 0;
  static unsigned long nextRadioInfoTime = 0;

  // some internal static values for parsing the input
  static char command;
  static int16_t value;
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;

  char c;
  if (Serial.available() > 0) {
    // read the next char from input.
    c = Serial.peek();

    if ((state == STATE_PARSECOMMAND) && (c < 0x20)) {
      // ignore unprintable chars
      Serial.read();

    } else if (state == STATE_PARSECOMMAND) {
      // read a command.
      command = Serial.read();
      state = STATE_PARSEINT;

    } else if (state == STATE_PARSEINT) {
      if ((c >= '0') && (c <= '9')) {
        // build up the value.
        c = Serial.read();
        value = (value * 10) + (c - '0');
      } else {
        // not a value -> execute
        runSerialCommand(command, value);
        command = ' ';
        state = STATE_PARSECOMMAND;
        value = 0;
      } // if
    } // if
  } // if


  // check for RDS data
  radio.checkRDS();

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastf) {
      // print current tuned frequency
      DisplayFrequency(f);
      lastf = f;
    } // if
    nextFreqTime = now + 400;
  } // if  

} // loop

// End.