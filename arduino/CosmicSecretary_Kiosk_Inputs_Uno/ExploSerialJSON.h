/* 
  All packet exchanges shall be "call and response", in which the host issues a call and the client issues an appropriate response ASAP

  JSON packets have the following generic format:
  {
    TYPE: <type>
    ,PAYLOAD: <JSON payload>
    ,COMMENT: <printable string comment>
   }

   PAYLOAD is present in all types save PING and ACK and contains the data to be exchanged.  It may be a string, int or a more complex JSON, dependent on type
   COMMENT is optional for all types, and may be ignored or passed on to a debug console, at the programmer's discretion.
   
   The USB transmittted string version of the JSON packet will always terminate in \n, i.e. LF.
 
 */

#include <ArduinoJson.h>
#define PACKET_SIZE 100  // static JSON object size, consult website to calculate
//typedef StaticJsonDocument<PACKET_SIZE>* (*UploadCallback) (void);  // the upload callback will return a pointer to a local "global" JSON payload buffer
typedef String (*UploadCallback) (void);
typedef int (*DownloadCallback) (String payload);

#define EXPLO_SERIAL_JSON_VERSION "ExploSerialJson V6.0 RJG 5/2/21"
 
// Macros for constructing and parsing packets
// top level name fields
#define TYPE "T"  // present in all packets
#define PAYLOAD "PL"  // May be any type of JSON, dependent on type. Expected for all but PING, ACK.  
#define COMMENT "C"  // Always a printable string. Optional in any type.

//  Packet types
#define PING "P"  // host call for ACK
/*
  {
    TYPE: PING,
    COMMENT: "This is a PING from host to client"
  }
 */
#define ACK "A"   // client response to PING
/*
  {
    TYPE: ACK,
    COMMENT: "This is an ACK from client to host"
  }
 */
#define ERR "E"   // client response to bad call packet 
/*
  {
    TYPE: ERR,
    PAYLOAD: <A printable string describing the error type>,
    COMMENT: "This is a error message from client to host"
  }
 */  
#define UPLOAD "U"  // call is request, response carries data from client to host
/*
  call
  {
    TYPE: UPLOAD,
    COMMENT: "This is a data transfer request from host to client"
  }
   response
  {
    TYPE: UPLOAD,
    PAYLOAD: {<any type of JSON, independent of protocol, specific to applicatiton>},
    COMMENT: "This is a data transfer from client to host"
  }
 */  
#define DOWNLOAD "D"  // call carries data from host to client, response acks data
/*
  call
  {
    TYPE: DOWNLOAD,
    PAYLOAD: {<any type of JSON, independent of protocol, specific to applicatiton>},
    COMMENT: "This is a data transfer from host to client"
  }
  response (good payload)
  {
    TYPE: DOWNLOAD,
    COMMENT: "This acknowleges a successful data transfer from host to client"
  }
    response (bad payload)
  {
    TYPE: ERR,
    PAYLOAD: BAD_PAYLOAD,
    COMMENT: "This reports an invalid payload in data transfer from host to client"
  }
 */ 
#define VERSION "V"  // call is request, response carries data from client to host
/*
  call
  {
    TYPE: VERSION,
    COMMENT: "This is a version request from host to client"
  }
  response 
  {
    TYPE: VERSION,
    PAYLOAD: <version string> 
    COMMENT: "This is a version response from client to host"
  }
 */ 
 #define BOARD_ADDRESS "B"  // call is request, response carries data from client to host
/*
  call
  {
    TYPE: BOARD_ID,
    COMMENT: "This is a board address request from host to client"
  }
  response 
  {
    TYPE: BOARD_ID,
    PAYLOAD: <integer or string, app programmer's choice> (Note: could be a null string),
    COMMENT: "This is a board address response from client to host"
  }
 */ 

// error codes
#define BAD_PACKET "packet failed to parse"
#define BAD_TYPE "unrecognized packet type"
#define BAD_CALLBACK "no callback function attached"
#define BAD_PAYLOAD "payload JSON failed to parse"
#define BAD_ADDRESS "board address undefined"

// other whitespace ASCII macros
#define CR 13  // in case one gets mixed in
#define LF 10  // Packets should terminate with LF  
#define NULL_CHAR  0 // C string terminator

// library class

class ExploSerial
{
  public:
    ExploSerial();
    int pollSerial(void);
    void setAddress(int addr);
    void setVersion(String ver);
    void attachUploadCallback(UploadCallback callback);
    void attachDownloadCallback(DownloadCallback callback);
  private:    
    int getCall(void);
    void processCall(void);
    void sendAck(void);
    void sendVersion(void);
    void sendAddr(void);
    void sendUpload(void);
    void processDownload(void);
    void sendPacket(void);
    void sendError(String thisError);
    StaticJsonDocument<PACKET_SIZE> PacketBuff;
    int boardAddress;
    String sketchVersion;
    UploadCallback thisUploadCallback;
    DownloadCallback thisDownloadCallback;
};
