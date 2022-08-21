#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "ExploSerialJSON.h"

ExploSerial::ExploSerial()
{
  boardAddress = -1;  // illegal address is also a flag that the address is not set yet
  sketchVersion = "";  // empty string garantees a non-null compound version string
  PacketBuff = {};
  thisUploadCallback = NULL;
  thisDownloadCallback = NULL;
}

int ExploSerial::pollSerial(void)
{
  int callStatus = getCall();
  if (callStatus == 1) processCall();
  else if (callStatus == -1) {
    sendError(BAD_PACKET);
  }
  return(callStatus);
}

void ExploSerial::processCall(void)
{
  String type = PacketBuff[TYPE]; 
  if (type == PING) sendAck();
  else if (type == VERSION) sendVersion();
  else if (type == BOARD_ADDRESS) sendAddr();
  else if (type == UPLOAD) sendUpload();
  else if (type == DOWNLOAD) processDownload();
  else sendError(BAD_TYPE);
  // dump JSON rx buffer?
}

int ExploSerial::getCall(void)
{ 
  int gotCall = 0;
  if (SerialUSB.available()) {
//    String temp = SerialUSB.readStringUntil(LF);
//    SerialUSB.println(temp);
    DeserializationError error = deserializeJson(PacketBuff, SerialUSB);
    if (error) {
      #ifdef DEBUG   
      SerialUSB.print(F("deserializeJson() failed: "));
      SerialUSB.println(error.f_str());
      #endif
      gotCall = -1;  // flag bad packet
    }
    else {
      #ifdef DEBUG   
      serializeJson(PacketBuff, SerialUSB);  // echo packet
      SerialUSB.println();
      #endif
      gotCall = 1;
    }   
    // dump UART rx buffer?
    SerialUSB.println("clearing buffer");
    while(SerialUSB.available()) {
      char garbage = SerialUSB.read();
    }
  }
  return gotCall;
}

void ExploSerial::sendAddr(void)
{
  if (boardAddress >= 0) {
    PacketBuff[TYPE] = BOARD_ADDRESS;
    PacketBuff[PAYLOAD] = boardAddress;
//    PacketBuff[COMMENT] = "valid address";
    sendPacket();
  }
  else sendError(BAD_ADDRESS);
}

void ExploSerial::sendAck(void)
{
  PacketBuff = {};
  PacketBuff[TYPE] = ACK;
  sendPacket();
}

void ExploSerial::sendError(String thisError)
{
  PacketBuff = {};
  PacketBuff[TYPE] = ERR;
  PacketBuff[PAYLOAD] = thisError;
  sendPacket();
}

void ExploSerial::sendVersion(void)
{
  PacketBuff = {};
  PacketBuff[TYPE] = VERSION;
  PacketBuff[PAYLOAD] = sketchVersion + "; " + EXPLO_SERIAL_JSON_VERSION;
  sendPacket();
}

void ExploSerial::sendPacket(void)
{
  serializeJson(PacketBuff, SerialUSB);
  SerialUSB.write(LF);  // need a LF as a final packet delimiter
  PacketBuff = {};// clean up output buffer
}

void ExploSerial::setAddress(int addr)
{
  boardAddress = addr;
}

void ExploSerial::setVersion(String ver)
{
  sketchVersion = ver;
}

void ExploSerial::sendUpload(void)
{
  if (thisUploadCallback == NULL) sendError(BAD_CALLBACK);
  else {
    PacketBuff = {};
    PacketBuff[TYPE] = UPLOAD;
  //  PacketBuff[PAYLOAD] = "Some JSON";
    PacketBuff[PAYLOAD] = thisUploadCallback();
    sendPacket();
  }
}

void ExploSerial::processDownload(void)
{

  if (thisDownloadCallback == NULL) sendError(BAD_CALLBACK);
  else {
      String localBuff = PacketBuff[PAYLOAD];
      if (thisDownloadCallback(localBuff) < 0) sendError(BAD_PAYLOAD); // if payload is not accepted by application, reject payload
      else {  // download packed was good, ack it.
      PacketBuff = {};
      PacketBuff[TYPE] = DOWNLOAD;
      sendPacket();
    }
  }
}


void ExploSerial::attachUploadCallback(UploadCallback callback)
{
  thisUploadCallback = callback;
}

void ExploSerial::attachDownloadCallback(DownloadCallback callback)
{
  thisDownloadCallback = callback;
}
