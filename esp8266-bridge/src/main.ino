#include <ESP8266WiFi.h>
#include <ModbusTCP.h>
#include <ModbusRTU.h>
#include "wifi.h"

//static ModbusRTU rtu;
static ModbusTCP tcp;
static HardwareSerial _log = Serial;
static const int MODBUS_TCP_PORT = 502;

// static uint16_t transRunning = 0;  // Currently executed ModbusTCP transaction
// static uint8_t slaveRunning = 0;   // Current request slave
 
// static bool cbTcpTrans(Modbus::ResultCode event, uint16_t transactionId, void* data) { // Modbus Transaction callback
//   if (event != Modbus::EX_SUCCESS)                  // If transaction got an error
//     log.printf("Modbus result: %02X, Mem: %d\n", event, ESP.getFreeHeap());  // Display Modbus error code (222527)
//   if (event == Modbus::EX_TIMEOUT) {    // If Transaction timeout took place
//     tcp.disconnect(tcp.eventSource());          // Close connection
//   }
//   return true;
// }

static Modbus::ResultCode errResponse() {
    _log.printf("send failed\n");
    int slaveId = 3;
    Modbus::FunctionCode code = Modbus::FunctionCode::FC_READ_INPUT_REGS;
    tcp.errorResponce(slaveId, code, Modbus::EX_DEVICE_FAILED_TO_RESPOND); // Send exceptional response to master if request bridging failed
    return Modbus::EX_DEVICE_FAILED_TO_RESPOND; // Stop processing the frame
}

// Callback receives raw requests 
static Modbus::ResultCode cbTcpRaw(uint8_t* data, uint8_t len, void* custom) {
  auto src = (Modbus::frame_arg_t*) custom;
  _log.printf("TCP req: %d, Fn: %02X, len: %d, ", src->slaveId, data[0], len);
  // Save transaction ans slave it for response processing
  //transRunning = rtu.rawRequest(it->ip, data, len, cbTcpTrans, it->unitId);
  //if (!transRunning) {
    // rawRequest returns 0 is unable to send data for some reason
    return errResponse(); 
  //}
  // _log.printf("transaction: %d\n", transRunning);
  // return Modbus::EX_SUCCESS; // Stop processing the frame
}

static bool onTcpConnected(IPAddress ip) {
  _log.print("TCP connected from: ");
  _log.print(ip);
  _log.print("\n");
  return true;
}

static bool onTcpDisconnected(IPAddress ip) {
  _log.print("TCP disconnected from: ");
  _log.print(ip);
  _log.print("\n");
  return true;
}

// // Callback receives raw data from ModbusTCP and sends it on behalf of slave (slaveRunning) to master
// Modbus::ResultCode cbRtuRaw(uint8_t* data, uint8_t len, void* custom) {
//   auto src = (Modbus::frame_arg_t*) custom;
//   log.print("RTU: Fn: %02X, len: %d \n", data[0], len);
//   if (!src->to_server && transRunning == src->transactionId) { // Check if transaction id is match
//     tcp.rawResponce(slaveRunning, data, len);
//     transRunning = 0;
//     slaveRunning = 0;
//     return Modbus::EX_SUCCESS; // Stop other processing
//   } else {
//     return Modbus::EX_PASSTHROUGH; // Allow frame to be processed by generic ModbusTCP routines
//   }
// }

void setup() {
  _log.begin(115200, SERIAL_8N1);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
    
  tcp.server(MODBUS_TCP_PORT); // Initialize ModbusTCP to process as server
  tcp.onRaw(cbTcpRaw); // Assign raw data processing callback
  tcp.onConnect(onTcpConnected);
  tcp.onDisconnect(onTcpDisconnected);
  
  // rtu.begin(&Serial);
  // rtu.server(); // Initialize ModbusRTU as server
  // rtu.onRaw(cbRtuRaw); // Assign raw data processing callback
}

void loop() {
  // rtu.task();
  tcp.task();
  yield();
}