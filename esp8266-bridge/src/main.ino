#include <ESP8266WiFi.h>
#include <ModbusTCP.h>
#include <ModbusRTU.h>
#include <TelnetStream.h>
#include "wifi.h"

// static ModbusRTU rtu;
static ModbusTCP tcp;
static TelnetStreamClass& _log = TelnetStream; // TX only
//static HardwareSerial& _rtuSerial = Serial;

static const int MODBUS_TCP_PORT = 502;
static uint16_t rtuTransId = 0;
static uint16_t tcpTransId = 0;
static uint32_t tcpIpaddr = 0;

// Callback that receives raw TCP requests 
static Modbus::ResultCode cbTcpRaw(uint8_t* data, uint8_t len, void* custom) {
  auto src = (Modbus::frame_arg_t*) custom;
  auto slaveId = src->slaveId;
  auto funCode = static_cast<Modbus::FunctionCode>(data[0]);

  // if (_log.available()) {
  //   _log.printf("TCP req: %d, Fn: %02X, len: %d\n: ", slaveId, funCode, len);
  // }

  // Must save transaction ans slave it for response processing
  //rtuTransId = rtu.rawRequest(slaveId, data, len);
  if (!rtuTransId) {
    // rawRequest returns 0 is unable to send data for some reason
    // if (_log.available()) {
    //   _log.printf("err\n");
    // }
    tcp.setTransactionId(src->transactionId); 
    tcp.errorResponce(IPAddress(src->ipaddr), funCode, Modbus::EX_DEVICE_FAILED_TO_RESPOND);
  } else {
    tcpTransId = src->transactionId;
    tcpIpaddr = src->ipaddr;
    // if (_log.available()) {
    //   _log.printf("rtuTransId: %d\n", rtuTransId);
    // }
  }
  
  // Stop other processing
  return Modbus::EX_SUCCESS; 
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

// Callback that receives raw responses
Modbus::ResultCode cbRtuRaw(uint8_t* data, uint8_t len, void* custom) {
  auto src = (Modbus::frame_arg_t*) custom;
  auto slaveId = src->slaveId;
  auto funCode = static_cast<Modbus::FunctionCode>(data[0]);

  // if (_log.available()) {
  //   _log.printf("RTU: Fn: %02X, len: %d, transId: %d, to_server: %d\n", funCode, len, src->transactionId, src->to_server);
  // }
  if (!src->to_server && rtuTransId == src->transactionId) { // Check if transaction id is match
    tcp.rawResponce(IPAddress(tcpIpaddr), data, len);
    rtuTransId = 0;
    tcpTransId = 0;
    tcpIpaddr = 0;
  } else {
    // if (_log.available()) {
    //   _log.printf("RTU: ignored, not in progress\n");
    // }
  }
  return Modbus::EX_SUCCESS; // Stop other processing
}

void setup() {
  //_log.begin(115200, SERIAL_8N1);
  
  // WiFi.mode(WIFI_STA);
  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);

  tcp.server(MODBUS_TCP_PORT);
  tcp.onRaw(cbTcpRaw);
  tcp.onConnect(onTcpConnected);
  tcp.onDisconnect(onTcpDisconnected);
  
  // rtu.begin(&_rtuSerial);
  // rtu.master();
  // rtu.onRaw(cbRtuRaw); 

  TelnetStream.begin();
}

void loop() {
  // Clear RX buffer
  while (_log.available() > 0) {
    _log.read();
  }

  //rtu.task();
  tcp.task();
  yield();
}