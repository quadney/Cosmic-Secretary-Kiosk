#define SKETCH_VERSION "Cosmic Secretary Kiosk - Outputs - Sydney Parcell 07/26/2022"
#define ARDUINO_ADDRESS 1

#include "ExploSerialJSON.h"

ExploSerial CommChannel;
StaticJsonDocument<PACKET_SIZE> DlJsonBuff = {};
String StringBuff = "";

#define J_COMMAND "C"
#define COMMS_MOTOR_1 200

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);///make into a #define
  while (!Serial);//////////////////remove
  setupComs();
}

void loop() {
  CommChannel.pollSerial();
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
  int command = DlJsonBuff[J_COMMAND];
  if (command == COMMS_MOTOR_1) {
    // {"T":"D", "PL":"{\"C\":\"200\"}"}
    return 1;
  }

  return 0;
}
