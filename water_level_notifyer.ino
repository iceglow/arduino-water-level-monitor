#define FLEX_LEVEL_WARNING 475
#define FLEX_LEVEL_ALERT 400
#define ONBOARD_LED 13
#define FLEX_SENSOR 0

#define STATE_NORMAL  0
#define STATE_WARNING 1
#define STATE_ALERT   2

#include <WiFlyHQ.h>
#include <SoftwareSerial.h>

#include "config.h"

SoftwareSerial wifiSerial(2,3);
WiFly wifly;

byte bendState = STATE_NORMAL;
char buf[255];

void setup()
{
  // for serial window debugging
  Serial.begin(115200);
  // set pin for onboard led
  pinMode(ONBOARD_LED, OUTPUT);

  Serial.println(F("Starting"));
  Serial.print(F("Free memory: "));
  Serial.println(wifly.getFreeMemory(),DEC);

  wifiSerial.begin(9600);
  if (!wifly.begin(&wifiSerial, &Serial)) {
    Serial.println(F("Failed to start wifly"));
    wifly.terminal();
  }

  associate();

  wifly.setDeviceID("Wifly-WebServer");

  if (wifly.isConnected()) {
    Serial.println(F("Old connection active. Closing"));
    wifly.close();
  }

  wifly.setProtocol(WIFLY_PROTOCOL_TCP);

  if (wifly.getPort() != 80) {
      wifly.setPort(80);
    /* local port does not take effect until the WiFly has rebooted (2.32) */
    wifly.save();
    Serial.println(F("Set port to 80, rebooting to make it work"));
    wifly.reboot();
    delay(3000);
  }

  Serial.println(F("Ready"));
}

/**
 * Join wifi network if not already associated
 */
void associate() {

  if (!wifly.isAssociated()) {
    /* Setup the WiFly to connect to a wifi network */
    Serial.println(F("Joining network"));
    wifly.setSSID(SSID);
    wifly.setPassphrase(WPA_PASS);
    wifly.enableDHCP();
    wifly.save();

    if (wifly.join()) {
        Serial.println(F("Joined wifi network"));
    } else {
        Serial.println(F("Failed to join wifi network"));
        wifly.terminal();
    }
  } else {
      Serial.println(F("Already joined network"));
  }

  wifly.setBroadcastInterval(0);	// Turn off UPD broadcast

  //wifly.terminal();

  Serial.print(F("MAC: "));
  Serial.println(wifly.getMAC(buf, sizeof(buf)));
  Serial.print(F("IP: "));
  Serial.println(wifly.getIP(buf, sizeof(buf)));
}

int getBendValue()
{
  // poll FLEX_SENSOR voltage
  return analogRead(FLEX_SENSOR);
}

void sendWaterNormal(int bendValue)
{
  digitalWrite(ONBOARD_LED, LOW);
  Serial.print("Water Level Returned To Normal bendValue=");
  Serial.println(bendValue);
}

void sendWaterWarning(int bendValue)
{
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.print("Water Level Warning bendValue=");
  Serial.println(bendValue);
}

void sendWaterAlert(int bendValue)
{
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.print("Water Level Alert bendValue=");
  Serial.println(bendValue);
}

byte getBendState(int bendValue, byte currentState)
{
  byte newState = currentState;

  switch (currentState)
  {
    case STATE_NORMAL: // bendValue does not exceed high or low values
      if (bendValue <= FLEX_LEVEL_WARNING)
      {
        newState = STATE_WARNING;
        sendWaterWarning(bendValue);
      }
      break;
    case STATE_WARNING: // bendValue exceeds high or low values
      if (bendValue <= FLEX_LEVEL_ALERT)
      {
        newState = STATE_ALERT;
        sendWaterAlert(bendValue);
      }
      else if (bendValue > FLEX_LEVEL_WARNING)
      {
        newState = STATE_NORMAL;
        sendWaterNormal(bendValue);
      }
      break;
    case STATE_ALERT:
      if (bendValue > FLEX_LEVEL_ALERT) {
        newState = STATE_WARNING;
        sendWaterWarning(bendValue);
      }
      break;
  }

  return newState;
}

String getBendStateString(byte state) {
  switch (state) {
    case STATE_NORMAL:
      return "NORMAL";
    case STATE_WARNING:
      return "WARNING";
    case STATE_ALERT:
      return "ALERT";
    default:
      return "UNKNOWN";
  }
}

String getRequestURI(String request) {
  int uriStart = request.indexOf(' ') + 1;
  int uriEnd = request.lastIndexOf(' ');

  String uri = "";
  
  if (uriStart > 0) {
    if (uriEnd <= uriStart) {
      uriEnd = request.length();
    }
    uri = request.substring(uriStart, uriEnd);
  }

  return uri;
}

void handleRequest(String request) {
  Serial.println("Request: " + String(buf));
  
  String uri = getRequestURI(request);
  String value = String(getBendValue());
  String state = getBendStateString(bendState);
  
  if (uri == "/state") {
    sendResponse(state);
  } else if (uri == "/value") {
    sendResponse(value);
  } else if (uri == "/") {
    sendResponse("{\"state\":\"" + state + "\", \"value\":" + value + "}");
  } else {
    Serial.println("UNKNOWN URI: " + uri);
    send404();
  }
}

/**
 * Send 200 OK and response body
 */
void sendResponse(String response) {
  int retLen = response.length()+1;
  char retArr[retLen];
  response.toCharArray(retArr, retLen);

  wifly.println(F("HTTP/1.1 200 OK"));
  wifly.println(F("Content-Type: application/json;charset=utf-8"));
  wifly.println(F("Transfer-Encoding: chunked"));
  wifly.println();
  wifly.sendChunkln(retArr);
  wifly.sendChunkln();

  wifly.flushRx();
}

/**
 * Send a 404 error
 */
void send404() {
  Serial.println(F("Sending 404"));
  wifly.println(F("HTTP/1.1 404 Not Found"));
  wifly.println(F("Content-Type: text/html"));
  wifly.println(F("Transfer-Encoding: chunked"));
  wifly.println();
  wifly.sendChunkln(F("<html><head>"));
  wifly.sendChunkln(F("<title>404 Not Found</title>"));
  wifly.sendChunkln(F("</head><body>"));
  wifly.sendChunkln(F("<h1>Not Found</h1>"));
  wifly.sendChunkln(F("<hr>"));
  wifly.sendChunkln(F("</body></html>"));
  wifly.sendChunkln();
  wifly.flushRx();		// discard rest of input
}

void loop() {
  // wait 10ms each loop iteration
  delay(10);

  int bendValue = getBendValue();
  byte newState = getBendState(bendValue, bendState);
  
  if (newState != bendState) {
    Serial.println("State changed from " + getBendStateString(bendState) + " to " + getBendStateString(newState));
    
    bendState = newState;
  }

  // WiFly is available
  if (wifly.available() > 0) {

    /* See if there is a request */
    if (wifly.gets(buf, sizeof(buf))) {
      if (strncmp_P(buf, PSTR("GET "), 4) == 0 || strncmp_P(buf, PSTR("POST "), 5) == 0) {
        handleRequest(String(buf));
      } else {
        /* Unexpected request */
        Serial.print(F("Unexpected: "));
        Serial.println(buf);
        send404();
      }
    }
  }
}
