#include "connectionManager.h"
#include "time.h"
#include <array>

connectionManager ConnectionManager;

const char* ssid = "";
const char* password = "";
const String deviceId = "CableLamp";
const String deviceKey = "";

// CableLamp
const int lampEnablePin = 32;
//const int buttonPin     = 33;


boolean lampOn = false;
//bool buttonState = false;
//bool prevButtonState = false;


// Stern
const int transistorPin = 12;
//const int potPin        = 14;

const int transistorChannel = 0;
const int transistorFrequency = 200;
const int maxDutyCycle = 100;
int sternIntensity = 0;






// Get the time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

void onMessage(DynamicJsonDocument message) {
  String error = message["error"];
  String packetType = message["type"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "setLampState")
  {
    setLampState(message["data"]);
  } else if (packetType == "setSternIntensity") {
    setSternIntensity(message["data"]);
  } else if (packetType == "curState") {
    setLampState(message["data"]["lampOn"]);
    setSternIntensity(message["data"]["sternIntensity"]);
  }
}




void setup() {
  Serial.begin(115200);

  pinMode(lampEnablePin, OUTPUT);
  delay(2000);

  digitalWrite(lampEnablePin, LOW);
  ledcAttachPin(transistorPin, transistorChannel);
  ledcSetup(transistorChannel, transistorFrequency, 8);
  ledcWrite(transistorChannel, 50);



  Serial.setDebugOutput(true);

  delay(1000);
  Serial.println("Waking up...");
  delay(1000);

  
  ConnectionManager.defineEventDocs("["
                                    "{"
                                    "\"type\": \"lampStatus\","
                                    "\"data\": \"bool\","
                                    "\"description\": \"Whether the CableLamp is on or not.\""
                                    "},"
                                    "{"
                                    "\"type\": \"sternIntensity\","
                                    "\"data\": \"int 0-100\","
                                    "\"description\": \"The intensity of the Stern.\""
                                    "}"
                                    "]");
  ConnectionManager.defineAccessPointDocs("["
                                          "{"
                                          "\"type\": \"setLampState\","
                                          "\"data\": \"bool\","
                                          "\"description\": \"Turns the CableLamp on or off.\""
                                          "},"
                                          "{"
                                          "\"type\": \"setSternIntensity\","
                                          "\"data\": \"int 0-100\","
                                          "\"description\": \"Sets the intensity of the Stern.\""
                                          "}"
                                          "]");

  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}






unsigned int programStarterClock = 0;
void loop() {
  ConnectionManager.loop();
  
  // Stern
  ledcWrite(transistorChannel, sternIntensity);
}


void setLampState(bool turnLampOn) {
  String statusMessage = "{\"type\": \"lampStatus\", \"data\":";
  if (turnLampOn)
  {
    lampOn = true;
    digitalWrite(lampEnablePin, HIGH);
    statusMessage += "true";
  } else {
    lampOn = false;
    digitalWrite(lampEnablePin, LOW);
    statusMessage += "false";
  }

  statusMessage += "}";
  ConnectionManager.send(statusMessage);
}

void setSternIntensity(int intensity) {
  if (intensity < 0) intensity = 0;
  if (intensity > 100) intensity = 100;
  String statusMessage = "{\"type\": \"sternIntensity\", \"data\":";
  sternIntensity = intensity * 2.55;
  statusMessage += intensity;
  statusMessage += "}";
  ConnectionManager.send(statusMessage);
}
