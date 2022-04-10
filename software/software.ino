#include "connectionManager.h";
#include "time.h";
#include <array>;

connectionManager ConnectionManager;

const char* ssid = "";
const char* password = "";
const String deviceId = "";
const String deviceKey = "";

const int lampEnablePin = 32;
const int buttonPin     = 33;

boolean lampOn = false;
bool buttonState = false;
bool prevButtonState = false;


String executeLightProgramTime = "";
unsigned int waitUntilMillis = 0;

int curLightProgramIndex = -1;
const int maxProgramSize = 64;
std::array<int, maxProgramSize> curLightProgram;

String programTrigger = "";
std::array<int, maxProgramSize> lightProgram = {};




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
  } else if (packetType == "executeGivenProgram")
  {
    if (curLightProgramIndex != -1) return; // Can't start a new program when there's still one running
    for (int i = 0; i < maxProgramSize; i++)
    {
      int instruction = message["data"][i];
      curLightProgram[i] = instruction;
    }
    runCurLightProgram();
  } else if (packetType == "prepareProgram")
  {
    String trigger = message["data"]["trigger"].as<String>();

    for (int i = 0; i < maxProgramSize; i++)
    {
      int instruction = message["data"]["program"][i];
      lightProgram[i] = instruction;
    }
    programTrigger = trigger;

  } else if (packetType == "executePreparedProgram")
  {
    if (curLightProgramIndex != -1) return; // Can't start a new program when there's still one running
    curLightProgram = lightProgram;
    runCurLightProgram();
  }
}




void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(lampEnablePin, OUTPUT);
  digitalWrite(lampEnablePin, LOW);

  Serial.begin(115200);

  //  Serial.setDebugOutput(true);

  delay(1000);
  Serial.println("Waking up...");

  ConnectionManager.setup(ssid, password, deviceId, deviceKey, &onMessage);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

unsigned int programStarterClock = 0;
void loop() {

  ConnectionManager.loop();

  buttonState = digitalRead(buttonPin);
  if (prevButtonState != buttonState && buttonState)
  {
    setLampState(!lampOn);
    ConnectionManager.send("{\"type\": \"buttonPressed\"}");
  }
  prevButtonState = buttonState;



  // Time detector/program starter
  programStarterClock++;
  if (programStarterClock > 100000) programStarterClock = 0;
  if (programStarterClock == 0 && curLightProgramIndex == -1)
  {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      char timeChar[3];
      strftime(timeChar, 6, "%H:%M", &timeinfo);
      if (String(timeChar) == programTrigger)
      {
        curLightProgram = lightProgram;
        curLightProgramIndex = 0;
      }
    }
  }












  // Light program executer
  updateProgramExecutor();
}



void updateProgramExecutor() {
  if (curLightProgramIndex == -1 || waitUntilMillis > millis()) return;

  if (curLightProgramIndex % 2 == 0)
  {
    switch (curLightProgram[curLightProgramIndex])
    {
      case 0:
        curLightProgramIndex = -1; // -2 + 1 = -1;
        ConnectionManager.send("{\"type\": \"setStateByKey\", \"stateKey\": \"programRunState\", \"data\": false}");
        return;
        break;
      case 1:
        setLampState(true);
        break;
      case 2:
        setLampState(false);
        break;
      case 3:
        setLampState(!lampOn);
        break;
    }
  } else {
    waitUntilMillis = millis() + curLightProgram[curLightProgramIndex];
  }

  curLightProgramIndex++;
  if (sizeof(curLightProgram) / sizeof(int) < curLightProgramIndex)
  {
    curLightProgramIndex = -1;
    ConnectionManager.send("{\"type\": \"setStateByKey\", \"stateKey\": \"programRunState\", \"data\": false}");
  }
}



void setLampState(bool turnLampOn) {
  String statusMessage = "{\"type\": \"setStateByKey\", \"stateKey\": \"lampOn\", \"data\":";
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




void runCurLightProgram() {
  curLightProgramIndex = 0;
  ConnectionManager.send("{\"type\": \"setStateByKey\", \"stateKey\": \"programRunState\", \"data\": true}");
}
