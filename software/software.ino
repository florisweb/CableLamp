#include "connectionManager.h"
#include "time.h"
#include <array>

connectionManager ConnectionManager;

const char* ssid = "";
const char* password = "";
const String deviceId = "";
const String deviceKey = "";

// CableLamp
const int lampEnablePin = 32;
boolean lampOn = false;


// Stern
const int transistorPin = 12;

const int transistorChannel = 0;
const int transistorFrequency = 200;
const int maxDutyCycle = 100;
int sternIntensity = 0;
int prevClickSternIntensity = 0;


// Rotary encoder
const int rotDTPin = 32;
const int rotCLKPin = 33;
const int rotButtonPin = 25;



// Get the time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

void onMessage(DynamicJsonDocument message) {
  String error = message["error"];
  String packetType = message["type"];

  Serial.print("[OnMessage] Error: ");
  Serial.println(error);
  Serial.print("[OnMessage] type: ");
  Serial.println(packetType);

  if (packetType == "setLampState") {
    setLampState(message["data"]);
  } else if (packetType == "setSternIntensity") {
    setSternIntensity(message["data"]);
  } else if (packetType == "animateSternIntensity") {
    animateSternIntensity(message["data"][0], message["data"][1]);
  } else if (packetType == "curState") {
    setLampState(message["data"]["lampOn"]);
    setSternIntensity(message["data"]["sternIntensity"]);
  }
}




void setup() {
  Serial.begin(115200);

  pinMode(lampEnablePin, OUTPUT);
  pinMode(rotDTPin, INPUT);
  pinMode(rotCLKPin, INPUT);
  pinMode(rotButtonPin, INPUT_PULLUP);
  delay(2000);

  digitalWrite(lampEnablePin, LOW);
  ledcAttach(transistorPin, transistorFrequency, 8);
  ledcWrite(transistorPin, 50);



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
                                          "},"
                                          "{"
                                          "\"type\": \"animateSternIntensity\","
                                          "\"data\": \"[intensity int 0-100, int duration ms]\","
                                          "\"description\": \"Animates the intensity of the Stern to the provided value.\""
                                          "}"
                                          "]");

  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}






unsigned int programStarterClock = 0;
bool curStateCLK = false;
bool lastStateCLK = false;
bool prevRotButtonState = false;
void loop() {
  ConnectionManager.loop();

  // Stern
  ledcWrite(transistorPin, sternIntensity * 2.55);
  updateSternAnimation();


  bool curRotButtonState = digitalRead(rotButtonPin);
  if (prevRotButtonState != curRotButtonState && curRotButtonState == 0) 
  {
    if (sternIntensity == 0)
    {
      animateSternIntensity(prevClickSternIntensity, 150);
    } else {
      prevClickSternIntensity = sternIntensity;
      animateSternIntensity(0, 150);
    }
  }
  prevRotButtonState = curRotButtonState;

  curStateCLK = digitalRead(rotCLKPin);
  if (curStateCLK != lastStateCLK && curStateCLK == true) {
    if (digitalRead(rotDTPin) != curStateCLK) {
      setSternIntensity(sternIntensity + 10);
    } else {
      setSternIntensity(sternIntensity - 10);
    }
  }
  lastStateCLK = curStateCLK;

  delay(1);
}



unsigned int sternAnimationStart = millis();
int sternAnimationDuration = 0;
int animateSternTo = 0;
int animateSternFrom = 0;
void updateSternAnimation() {
  if (sternAnimationDuration == 0) return;
  if (millis() - sternAnimationStart >= sternAnimationDuration)  // Finished animation
  {
    sternAnimationDuration = 0;
    setSternIntensity(animateSternTo);
  } else {
    float timePerc = (millis() - sternAnimationStart) * 100.0 / sternAnimationDuration;
    sternIntensity = round(animateSternFrom * (100.0 - timePerc) / 100.0 + animateSternTo * timePerc / 100.0);
  }
}

void animateSternIntensity(int intensity, int duration) {
  if (intensity < 0) intensity = 0;
  if (intensity > 100) intensity = 100;

  sternAnimationStart = millis();
  sternAnimationDuration = duration;
  animateSternTo = intensity;
  animateSternFrom = sternIntensity;
}

void setLampState(bool turnLampOn) {
  String statusMessage = "{\"type\": \"lampStatus\", \"data\":";
  if (turnLampOn) {
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
  sternIntensity = intensity;
  statusMessage += intensity;
  statusMessage += "}";
  ConnectionManager.send(statusMessage);
}
