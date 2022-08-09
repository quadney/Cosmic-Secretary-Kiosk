#include <Adafruit_WS2801.h>

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

uint32_t colors[] = {0, 0, 0, 0, 0};

//// LED THINGS

#define INTUITION_INDEX 0
#define LOVE_INDEX 3
#define COMMUNICATION_INDEX 5

#define EARTH_INDEX 13
#define AIR_INDEX 11
#define WATER_INDEX 9
#define FIRE_INDEX 7

int elementIndices[] = {EARTH_INDEX, AIR_INDEX, WATER_INDEX, FIRE_INDEX };

uint8_t dataPin  = 2;    // Yellow wire on Adafruit Pixels
uint8_t clockPin = 3;    // Green wire on Adafruit Pixels

#define NUM_LEDS 25

Adafruit_WS2801 strip = Adafruit_WS2801(NUM_LEDS, dataPin, clockPin);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);///make into a #define
  while (!Serial);//////////////////remove
  setupComs();

  strip.begin();
  // Update LED contents, to start they are all 'off'
  strip.show();

  setupColors();
}

void loop() {
  CommChannel.pollSerial();

  // update LEDs
}

void setupColors() {
  colors[M_DARK] = Color(0,0,0);
  colors[M_EARTH] = Color(0, 100, 0);
  colors[M_AIR] = Color(255, 87, 51);
  colors[M_WATER] = Color(3,37,126);
  colors[M_FIRE] = Color(226,82,47);
  colors[M_COMPATABLE] = Color(255, 255, 255);
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
  Serial.println(lightValue);
  if (lightValue > 100 && lightValue < 500) {
    // {"T":"D", "PL":"{\"L\":\"111\"}"}
    updateForMedallion(INTUITION_INDEX, lightValueString.substring(D_INTUITION, D_LOVE));
    updateForMedallion(LOVE_INDEX, lightValueString.substring(D_LOVE, D_COMMUNICATION));
    updateForMedallion(COMMUNICATION_INDEX, lightValueString.substring(D_COMMUNICATION));
    
    return 1;
  }

  return 0;
}

void updateForMedallion(int medallion_index, String colorIndex) {
  int element = colorIndex.toInt();
  // set the intuition medallion on 
  strip.setPixelColor(medallion_index, colors[element]);
  // set the corresponding element on
  strip.setPixelColor(elementIndices[element - 1], colors[element]);
  strip.show();
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
