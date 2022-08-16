#include <Adafruit_MPR121.h>

#include <Adafruit_miniTFTWing.h>
#include <seesaw_servo.h>
#include <Adafruit_NeoKey_1x4.h>
#include <Adafruit_Crickit.h>
#include <seesaw_neopixel.h>
#include <seesaw_spectrum.h>
#include <Adafruit_TFTShield18.h>
#include <seesaw_motor.h>
#include <Adafruit_NeoTrellis.h>
#include <Adafruit_seesaw.h>

#include <SparkFun_Alphanumeric_Display.h>
#include <Wire.h>

// COMMS 
#define SKETCH_VERSION "Cosmic Secretary - Input Kiosk - Sydney Parcell 07/26/2022"
#define ARDUINO_ADDRESS 1
#include "ExploSerialJSON.h"

ExploSerial CommChannel;
StaticJsonDocument<PACKET_SIZE> DlJsonBuff = {};
String StringBuff = "";

// ALPHANUMERIC DISPLAYS
HT16K33 monthDisplay;
HT16K33 yearDisplay;
HT16K33 dayDisplay;
HT16K33 timezoneDisplay;

// CAPACATIVE TOUCH SENSOR
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t lasttouched = 0;
uint16_t currtouched = 0;
uint16_t TOUCH_PIN = 5;
#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

// ENCODERS
#define SS_SWITCH 24      // this is the pin on the encoder connected to switch
#define SEESAW_BASE_ADDR 0x36  // I2C address, starts with 0x36
// create 4 encoders!
Adafruit_seesaw encoders[4];
//TODO: make this an enum, forget how to do in arduino
#define MONTH_ENCODER 0
#define DAY_ENCODER 1
#define YEAR_ENCODER 2
#define TIMEZONE_ENCODER 3

int32_t encoder_positions[] = {0, 0, 0, 0};
bool found_encoders[] = {false, false, false, false};

// PROGRAM DATA
#define MONTHS 12
int currentMonthIndex = 0;
String months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

#define START_YEAR 1940
#define END_YEAR 2015
int currentYear = 1993;

#define START_DAY 1
#define END_DAY 30
#define END_DAY_FEB 28
#define END_DAY_FEB_LEAP 29
#define END_DAY_LONG 31
int currentDay = START_DAY;

#define TIMEZONES 9
int currentTimezoneIndex = 0;
String timezones[] = {"N-AM", "S-AM", "EURO", "AFRI", "AUST", "OCEA", "W-AS", "C-AS", "E-AS"};

void setup() {
  Serial.begin(115200);

  // wait for serial port to open
  while (!Serial) delay(10);

  // setup comms
  setupComs();

  // setup inputs
  setupEncoders();
  setupCapacativeTouchSensor();

  // setup outputs
  setupDisplays();
}

void loop() {
  CommChannel.pollSerial();
  
  uint16_t display_line = 1;

  // check capacative touch 
  checkCapSensor();

  // check encoders
  checkEncoders();

  // don't overwhelm serial port
  yield();
  delay(10);
}

//// LOOPING METHODS
void checkCapSensor() {
  // Get the currently touched pads
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) && i == TOUCH_PIN) {
      sendComputerCurrentData();
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) && i == TOUCH_PIN) {
     sendComputerEndData();
    }
  }

  // reset our state
  lasttouched = currtouched;
}

void checkEncoders() {
  for (uint8_t enc = 0; enc<sizeof(found_encoders); enc++) { 
     if (found_encoders[enc] == false) continue;
  
     int32_t new_position = encoders[enc].getEncoderPosition();
     // did we move around?
     if (encoder_positions[enc] != new_position) {
       int deltaChange = encoder_positions[enc] - new_position;
       // compare previous encoder value to new
       if (enc == MONTH_ENCODER) {
         currentMonthIndex += deltaChange;
         currentMonthIndex = constrain(currentMonthIndex, 0, MONTHS - 1);
         checkCurrentDay();
         updateMonthDisplay();
       }
       else if (enc == YEAR_ENCODER) {
        currentYear += deltaChange;
        currentYear = constrain(currentYear, START_YEAR, END_YEAR);
        checkCurrentDay();
        updateYearDisplay();
       }
       else if (enc == TIMEZONE_ENCODER) {
         currentTimezoneIndex += deltaChange;
         currentTimezoneIndex = constrain(currentTimezoneIndex, 0, TIMEZONES - 1);
         updateTimezoneDisplay();
       }
       else if (enc == DAY_ENCODER) {
        currentDay += deltaChange;
         checkCurrentDay();
       }
       encoder_positions[enc] = new_position;
     }
  }
}

void checkCurrentDay() {
  String currentMonth = months[currentMonthIndex];
  if (currentMonth.equals("APR") || currentMonth.equals("JUN") || 
      currentMonth.equals("SEP") || currentMonth.equals("NOV")) {
    currentDay = constrain(currentDay, START_DAY, END_DAY);
  }
  else if (currentMonth.equals("FEB")) {
    if (currentYear % 4 == 0) {
      currentDay = constrain(currentDay, START_DAY, END_DAY_FEB_LEAP);
    }
    else {
      currentDay = constrain(currentDay, START_DAY, END_DAY_FEB);
    }
  }
  else {
    currentDay = constrain(currentDay, START_DAY, END_DAY_LONG);
  }
  updateDayDisplay();
}

//// SETUP METHODS
void setupEncoders() {
  for (uint8_t enc=0; enc<sizeof(found_encoders); enc++) {
    // See if we can find encoders on this address 
    if (! encoders[enc].begin(SEESAW_BASE_ADDR + enc)) {
      Serial.print("Couldn't find encoder #");
      Serial.println(enc);
    } 
    else {
      uint32_t version = ((encoders[enc].getVersion() >> 16) & 0xFFFF);
      if (version != 4991){
        Serial.print("Wrong firmware loaded? ");
        Serial.println(version);
        while(1) delay(10);
      }
  
      // use a pin for the built in encoder switch
      encoders[enc].pinMode(SS_SWITCH, INPUT_PULLUP);

      // get starting position
      encoder_positions[enc] = encoders[enc].getEncoderPosition();
  
      delay(10);
      encoders[enc].setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
      encoders[enc].enableEncoderInterrupt();

      found_encoders[enc] = true;
    }
  }
}

void setupCapacativeTouchSensor() {
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
}

void setupDisplays() {
  Wire.begin(); //Join I2C bus
  if (monthDisplay.begin(0x70) == false) {
    Serial.println("Device did not acknowledge month dsiplay! Freezing.");
    while(1);
  }
  if (dayDisplay.begin(0x71) == false) {
    Serial.println("Device did not acknowledge day display! Freezing.");
    while(1);
  }
  if (yearDisplay.begin(0x72) == false) {
    Serial.println("Device did not acknowledge year display! Freezing.");
    while(1);
  }
  if (timezoneDisplay.begin(0x73) == false) {
    Serial.println("Device did not acknowledge timezone display! Freezing.");
    while(1);
  }
  
  monthDisplay.setBrightness(1);
  yearDisplay.setBrightness(1);
  dayDisplay.setBrightness(1);
  timezoneDisplay.setBrightness(1);

  updateMonthDisplay();
  updateYearDisplay();
  updateDayDisplay();
  updateTimezoneDisplay();
}


////DISPLAY METHODS
void updateMonthDisplay() {
  monthDisplay.clear();
  monthDisplay.print(months[currentMonthIndex]);
}

void updateYearDisplay() {
  yearDisplay.clear();
  yearDisplay.print(currentYear);
}

void updateDayDisplay() {
  dayDisplay.clear();
  dayDisplay.print(currentDay);
}

void updateTimezoneDisplay() {
  timezoneDisplay.clear();
  timezoneDisplay.print(timezones[currentTimezoneIndex]);
}

//// ENCODER METHODS
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

//// COMMS METHODS 
void sendComputerEndData() {
  Serial.print(ARDUINO_ADDRESS);
  Serial.println(";");
}
void sendComputerCurrentData()
{
  Serial.print(ARDUINO_ADDRESS);
  Serial.print("/");
  Serial.print(currentMonthIndex + 1);
  Serial.print("/");
  Serial.print(currentDay);
  Serial.print("/");
  Serial.print(currentYear);
  Serial.print("/");
  Serial.print(timezones[currentTimezoneIndex]);
  Serial.println("");
}

// COMMS METHODS
void setupComs() {
  //Explo JSON stuff
  CommChannel.setAddress(ARDUINO_ADDRESS);
  CommChannel.setVersion(SKETCH_VERSION);
  CommChannel.attachUploadCallback(commUploadCallback);
  CommChannel.attachDownloadCallback(commDownloadCallback);
}

String commUploadCallback(void) {
  return ("{}");
}

int commDownloadCallback(String payload) {
  return 0;
}
