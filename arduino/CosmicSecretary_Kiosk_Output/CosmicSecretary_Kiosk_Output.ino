#include <FastLED.h>

#define SKETCH_VERSION "Cosmic Secretary Kiosk - Outputs - Sydney Parcell 07/26/2022"
#define ARDUINO_ADDRESS 2

#include "ExploSerialJSON.h"

ExploSerial CommChannel;
StaticJsonDocument<PACKET_SIZE> DlJsonBuff = {};
String StringBuff = "";

#define J_COMMAND "L"

#define D_INTUITION 0
#define D_LOVE 1
#define D_COMMUNICATION 2

#define M_DARK 0  // off
#define M_EARTH 1 // green
#define M_AIR 2   // light blue almost white
#define M_WATER 3 // dark blue
#define M_FIRE 4  // orange
#define M_COMPATABLE 5

//// LED THINGS

#define EARTH_HUE 96
#define AIR_HUE 128
#define FIRE_HUE 0
#define WATER_HUE 159
#define DEFAULT_HUE 190

#define INTUITION_INDEX 1
#define LOVE_INDEX 3
#define COMMUNICATION_INDEX 5

#define EARTH_INDEX 10
#define AIR_INDEX 9
#define WATER_INDEX 8
#define FIRE_INDEX 7

int allPins[] = { INTUITION_INDEX, LOVE_INDEX, COMMUNICATION_INDEX, EARTH_INDEX, AIR_INDEX, WATER_INDEX, FIRE_INDEX };
#define ALL_PINS 7

#define BREATHE_MIN 128;
#define BREATHE_MAX 255;
int breatheValue = BREATHE_MIN;
bool addBreathe = true;

#define DATA_PIN    2
#define CLK_PIN   3
#define LED_TYPE    WS2801
#define COLOR_ORDER RGB
#define NUM_LEDS 13
CRGB leds[NUM_LEDS];

//            INTUITION, LOVE,  COMMUNICATION, EARTH, AIR,   FIRE,  WATER
bool isOn[] = { false, false, false,  false, false, false, false };
int hue[] =   { EARTH_HUE, EARTH_HUE, EARTH_HUE, EARTH_HUE, AIR_HUE, FIRE_HUE, WATER_HUE };

#define INTUITION 0
#define LOVE 1
#define COMMUNICATION 2
#define EARTH 3
#define AIR 4
#define FIRE 5
#define WATER 6

bool isAttractMode = true;
bool isCompatability = false;
int cycleHueIndex = INTUITION;

void setup() {
  Serial.begin(115200);///make into a #define
  while (!Serial);//////////////////remove
  setupComs();

  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);


  // turn off all LEDS
  FastLED.clear();
  FastLED.setBrightness(128);

  delay(1000);

  // turn on element
  turnOnElements(true);
}

void loop() {
  CommChannel.pollSerial();

  // update LEDs
  for(int i = 0; i < ALL_PINS; i++) {
    int val = isOn[i] ? 255 : 0;
    if (i >= EARTH || isCompatability) {
      val = isOn[i] ? breatheValue : 0; 
    }
    leds[allPins[i]] = CHSV(hue[i], isCompatability ? 0 : 187, val);
  }
  FastLED.show();

  EVERY_N_MILLISECONDS( 10 ) { 
    updateBreatheValue(); 
  }
  EVERY_N_MILLISECONDS( 600 ) { 
    cycleMedallionLEDs(); 
  }  
}

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
  DlJsonBuff = {};
  deserializeJson(DlJsonBuff, payload);
  int lightValue = DlJsonBuff[J_COMMAND];
  String lightValueString = String(lightValue);
  if (lightValue == 999) {
    // set everything back to default 
    isAttractMode = true;
    turnOnElements(true);
    isCompatability = false;
  }
  else if (lightValue > 100 && lightValue < 500) {
    // {"T":"D", "PL":"{\"L\":\"111\"}"}
    isAttractMode = false;
    turnOnElements(false);
    isCompatability = false;
    animateBeforeShowing();
    updateForMedallion(INTUITION, lightValueString.substring(D_INTUITION, D_LOVE));
    updateForMedallion(LOVE, lightValueString.substring(D_LOVE, D_COMMUNICATION));
    updateForMedallion(COMMUNICATION, lightValueString.substring(D_COMMUNICATION));
    
    return 1;
  }
  else if (lightValue > 500) {
    // compatability function
    isAttractMode = false;
    turnOnElements(false);
    isCompatability = true;
    animateBeforeShowing();
    updateForMedallion(INTUITION, lightValueString.substring(D_INTUITION, D_LOVE));
    updateForMedallion(LOVE, lightValueString.substring(D_LOVE, D_COMMUNICATION));
    updateForMedallion(COMMUNICATION, lightValueString.substring(D_COMMUNICATION));
  }

  return 0;
}

void animateBeforeShowing() {
  // do 3 cycles of white
  int i = 0; 
  for (int j = 0; j < 6; j++) {
    leds[INTUITION_INDEX] = CHSV(0, 0, i == 0 ? 255 : 0);
    leds[LOVE_INDEX] = CHSV(0, 0, i == 1 ? 255 : 0);
    leds[COMMUNICATION_INDEX] = CHSV(0, 0, i == 2 ? 255 : 0);
    i++;
    i %= 3;
    FastLED.show();
    delay(150);
  }
}

void updateForMedallion(int medallion_index, String colorIndex) {
  int dataIndex = colorIndex.toInt();
  if (dataIndex == 9) {
    // turn off this medallion
    isOn[medallion_index] = false;
    return;
  }
  else if (dataIndex == 5) {
    isOn[medallion_index] = true;
  }
  else {
    // set the medallion on 
    hue[medallion_index] = hueForIndex(dataIndex);
    isOn[medallion_index] = true;
    
    // set the corresponding element on
    int elementIndex = elementForIndex(dataIndex);
    isOn[elementIndex] = true;
  }
}

int elementForIndex(int colorIndex) {
  switch(colorIndex) {
    case M_EARTH:
      return EARTH;
    case M_AIR: 
      return AIR;
    case M_WATER: 
      return WATER;
    case M_FIRE: 
      return FIRE;
  }
}

int hueForIndex(int colorIndex) {
  switch(colorIndex) {
    case M_EARTH:
      return EARTH_HUE;
    case M_AIR: 
      return AIR_HUE;
    case M_WATER: 
      return WATER_HUE;
    case M_FIRE: 
      return FIRE_HUE;
  }

  return DEFAULT_HUE;
}

void turnOnElements(bool on) {
  isOn[EARTH] = on;
  isOn[AIR] = on;
  isOn[FIRE] = on;
  isOn[WATER] = on;
}

void turnOnMedallions(bool on) {
  isOn[INTUITION] = on;
  isOn[LOVE] = on; 
  isOn[COMMUNICATION] = on;
}

void updateBreatheValue() {
  if (addBreathe) {
    breatheValue++;
  }
  else {
    breatheValue--;
  }

  if (breatheValue >= 255) {
    addBreathe = false;
  }
  else if (breatheValue <= 100) {
    addBreathe = true;
  }
}

void cycleMedallionLEDs() {
  if (isAttractMode) {
    turnOnMedallions(false);
    cycleHueIndex++;
    if (cycleHueIndex > COMMUNICATION) {
      cycleHueIndex = INTUITION;
    }
    hue[cycleHueIndex] = DEFAULT_HUE;
    isOn[cycleHueIndex] = true;
  }
}

void setAttractMode(bool isInAttractMode) {
  isAttractMode = isInAttractMode; 
}
