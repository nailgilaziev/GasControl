#include <Arduino.h>
/*
 * Известные проблемы, которые нужно будет посмотреть позже и проанализировать.
 * rotary encoder - когда останавливаешь вращение иногда происходят лишние
 * срабатывания. либо когда очень медленно по одному пункту ведешь и в конце
 * вращения происходит срабатывание несколько раз. Это нельзя оставлять в
 * продакшене - изучить возможности bounce2
 *
 * Когда идет запрос температуры - перестают работать органы управления(как
 * сделать асинхронное поведение)
 *
 * После позиционирования мотора снимать нагрузку с катушки -> потребляет ток в
 * 0.32А МОТОР СИЛЬНО ГРЕЕТСЯ - НЕ ОСТАВЛЯТЬ БЕЗ ФИКСА
 */
#include "ProjectData.h"
#include "Global.hpp"
#include "SerialRouter.hpp"
#include <AccelStepper.h>
#include "NokiaDisplay.h"
#include <Bounce2.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SimpleDHT.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define WARNING_NOT_STARTED_YET 2

SoftwareSerial gsmSerial(11, 9); // RX, TX

SerialRouter router(&gsmSerial);

/*
#define A0 D0
#define A1 D1
#define A2 D2
#define A3 D3
#define A5 D4
*/

/*
 * from uiState 1 to uiState 0 system will go automativally after few seconds
 */
unsigned long lastInteractionWithUi = 0;
#define INTERACTION_TIME 8000

// const int pBackButton = A3;
const int pButton = 2;
const int pRotaryA = 6;
const int pRotaryB = 4;

Bounce buttonDebouncer = Bounce();
Bounce rotaryDebouncerA = Bounce();
// Bounce rotaryDebouncerB = Bounce();
// not necessary for board with hardware debouncing
// Bounce backButtonDebouncer = Bounce();

// Data from sensors measured by intervals
unsigned long lastMeasureTime = -28000; //diff between this and MEASURE_INTERVAL is for DHT22
unsigned long lastSendTime = 0;
// 1800000
#define MEASURE_INTERVAL 30000
#define SEND_INTERVAL 300000
#define FIRST_SEND_INTERVAL 60000

const int pDHT = 5;
SimpleDHT22 dht22;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 3
// Setup a oneWire instance to communicate with any OneWire devices (not just
// Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// 4,2,1,3
AccelStepper stepper(4, 18, 14, 12, 16);
int8_t rawData[8];

NokiaDisplay display = NokiaDisplay(rawData);

void updateDispay() { display.update(); }
/* UI mode levels:
 * 0 is displaying temperature and ready to rotate stepdriver
 * (pressing rotary button or sensor button switch on display light)
 * 1 is the same that 0 - but with display brightning
 * (pressing rotary button open menu mode; pressing back button switch off
 * display)
 * 2 is the menu mode (pressing rotary buttom apply menu or go deeper;
 * back to back)
 */
int8_t getUiState() { return rawData[DISPLAY_MODE]; }
void setUiState(int8_t v) {
  rawData[DISPLAY_MODE] = v;
  updateDispay();
}

// 0 - while reading
// 1 - completed results
// 1+ - error codes
byte executionStatusCode = WARNING_NOT_STARTED_YET;

void checkExecStatus() {
  if (executionStatusCode < 2)
    return;
  static unsigned long lastReportTime = 1;
  if (millis() - lastReportTime > 5000) {
    lastReportTime = millis();
    for (int i = 0; i < executionStatusCode; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
    }
  }
}

void savePosition(){
  EEPROM.write(0,rawData[GAS_TARGET_TEMP]);
}

byte getStoredPosition(){
  byte v = EEPROM.read(0);
  if (v == 255) {
    EEPROM.write(0,0);
    return 0;
  }
  return v;
}


#define TEMP_TO_STEP 40

void setup(void) {
  // router.eventsListener.events = NULL;
  // start serial port
  for (int k = 0; k < 8; k++) {
    rawData[k] = 0; //-10 is error/not initialized state now it 127
  }
  rawData[ROOM_TEMP] = 127;
  rawData[ROOM_HUMIDITY] = 127;
  rawData[GAS_TARGET_TEMP] = getStoredPosition();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.println(F("Started"));

  gsmSerial.begin(9600);
  display.init();

  pinMode(pButton, INPUT); // INPUT_PULLUP - not pulled with builtin resistor
  //                          // bacause pulled with external resistor
  pinMode(pRotaryA, INPUT);
  pinMode(pRotaryB, INPUT);

  // backButtonDebouncer.attach(pBackButton);
  // backButtonDebouncer.interval(5);
  buttonDebouncer.attach(pButton);
  buttonDebouncer.interval(5);
  rotaryDebouncerA.attach(pRotaryA);
  rotaryDebouncerA.interval(1);
  // rotaryDebouncerB.attach(pRotaryA);
  // rotaryDebouncerB.interval(1);

  // Start up the library
  sensors.begin();
  executionStatusCode = 1;

  stepper.setAcceleration(200);
  stepper.setMaxSpeed(500);
  stepper.setCurrentPosition(rawData[GAS_TARGET_TEMP]*TEMP_TO_STEP);
}

boolean needAutomaticallySwitchOffDisplay() {
  return getUiState() == DISPLAY_ON &&
         millis() - lastInteractionWithUi > INTERACTION_TIME;
}

void switchOffDisplay() {
  setUiState(DISPLAY_OFF);
  lastInteractionWithUi = millis();
}

void switchOnDisplay() {
  setUiState(DISPLAY_ON);
  lastInteractionWithUi = millis();
  // Serial.println("Display on");
}

void showMenu() { setUiState(DISPLAY_MENU); }

void menuPressed() { setUiState(DISPLAY_ON); }

void menuBack() {
  // Serial.println("Back pressed on Menu item");
  setUiState(DISPLAY_ON);
}



bool stepperRunned = false;
unsigned long lastTimeStepperRunned = 0;

void moveStepper() {
  stepperRunned = true;
  lastTimeStepperRunned = millis();
  Serial.println(rawData[GAS_TARGET_TEMP]);
  stepper.moveTo(rawData[GAS_TARGET_TEMP] * TEMP_TO_STEP);
  updateDispay();
  savePosition();
}

void disableStepper() {
  if (millis() - lastTimeStepperRunned > 10000) {
    stepper.disableOutputs();
    stepperRunned = false;
    Serial.println("steppers disabled");
  }
}

void changeValue(int t) {
  // Serial.println("modes:");
  // Serial.println(rawData[DISPLAY_MODE]);
  // Serial.println(rawData[SELECTED_MENU]);

  if (rawData[DISPLAY_MODE] == DISPLAY_MENU) {
    rawData[SELECTED_MENU] = constrain(rawData[SELECTED_MENU] + t, 0, 2);
  } else if (rawData[SELECTED_MENU] == SELECTED_MENU_AUTOMATIC) {
    rawData[CLIM_TARGET_TEMP] =
        constrain(rawData[CLIM_TARGET_TEMP] + t, -9, 40);
    Serial.println(rawData[CLIM_TARGET_TEMP]);
  } else {
    rawData[GAS_TARGET_TEMP] = constrain(rawData[GAS_TARGET_TEMP] + t, -1, 90);
  }
  moveStepper();
  updateDispay();
}

void rotaryUp() {
  Serial.println("+");
  lastInteractionWithUi = millis();
  changeValue(+1);
}

void rotaryDown() {
  Serial.println("-");
  lastInteractionWithUi = millis();
  changeValue(-1);
}

bool measuredAfterStart = false;

boolean timeToSend() {
  if (!measuredAfterStart) {
    if (millis() > FIRST_SEND_INTERVAL) {
      measuredAfterStart = true;
      return true;
    }
    return false;
  }
  return millis() - lastSendTime > SEND_INTERVAL;
}

boolean timeToMeasure() {
  return millis() - lastMeasureTime > MEASURE_INTERVAL;
}

byte isBooted = 1;

bool internetSetupNeeded = true;

class DataReportCQ : public CmdsQueue {

  bool skipSetupProcess;
  int signalQualityRSSI = -1;
  int signalQualityBER = -1;

public:
  DataReportCQ(SerialRouter *sr) : CmdsQueue(sr) {
    skipSetupProcess = !internetSetupNeeded;
  }

  byte xiicAttempts = 0;
  CmdQisFinished newLineEvent(bool isFullLine) override {
    if (executedCmdIndex == 6) {
      if (responseIs("0.0.0.0", false)) {
        Serial.println();
        // Wait next OK and execute cmd again
        if (++xiicAttempts > 2) {
          xiicAttempts = 0;
          executedCmdIndex = 4;
        } else {
          executedCmdIndex = 5;
        }
        return false;
      }
    }
    if (executedCmdIndex == 8) {
      if (responseIs("+TCPSETUP:0,OK"))
        return cmdSuccseed();
      if (responseIs("+TCPSETUP:", false))
        return cmdFailed();
      return reactForSimpleLine();
    }
    if (executedCmdIndex == 9) {
      if (responseIs(">"))
        return cmdSuccseed();
      if (responseIs("+TCPSEND:", false))
        return cmdFailed();
      return reactForSimpleLine();
    }
    if (executedCmdIndex == 10) {
      if (responseIs("+TCPRECV:0", false))
        return cmdSuccseed();
      if (responseIs(":Error", false) || responseIs(":Buffer", false) ||
          responseIs(":Data", false))
        return cmdFailed();
      return reactForSimpleLine();
    }
    if (executedCmdIndex == 11) {
      if (responseIs("+TCPCLOSE:0,OK"))
        return cmdSuccseed();
      if (responseIs("+TCPCLOSE:", false))
        return cmdFailed();
      return reactForSimpleLine();
    }
    return CmdsQueue::newLineEvent(isFullLine);
  }

  // RING и +CMT: + следующую строку надо ловить и передавать в events listener
  // ATH release all calls
  const char *cmds[12] = {
      "AT+CPAS",                                   // 0
      "AT+CREG?",                                  // 1
      "AT+XISP=0",                                 // 2
      "AT+CGDCONT=1,\"IP\",\"internet.tele2.ru\"", // 3
      "AT+XGAUTH=1,1,\"\",\"\"",                   // 4
      "AT+XIIC=1",                                 // 5
      "AT+XIIC?",                                  // 6
      "AT+CSQ",                                    // 7
      "AT+TCPSETUP=0,184.106.153.149,80",          // 8
      "AT+TCPSEND=0,",                             // 9
      "GET /update?api_key=XB08GLN5246NL2K6&headers=false&"
      "field1=%i&field2=%i&field3=%i&field4=%i&field5=5%i&"
      "field6=%i&field7=%i&field8=%i HTTP/1.1\r\n"
      "Host: 184.106.153.149\r\n\r\n", // 10
      "AT+TCPCLOSE=0"                  // 11
  };

  CmdQisFinished cmdFailed() {
    Serial.println();
    executedCmdIndex = 255;
    internetSetupNeeded = true;
    executeCmd(getCmd(0));
    return false;
  }

  char urlBuffer[168];

  const char *getCmd(byte ind) override {
    if (ind == 0 && skipSetupProcess) {
      // if further in line problem exist, we NOT skip process
      skipSetupProcess = false;
      ind = 7;
      executedCmdIndex = 6; // after this call func var will be incremented
    }
    if (ind >= 12) {
      return NULL;
    }
    if (ind == 6)
      delay(6000); // delay for obtain IP
    if (ind == 11)
      delay(10); // and RECV Data
    if (ind != 9 && ind != 10)
      return cmds[ind];

    sprintf(urlBuffer, cmds[10], rawData[GAS_TEMP_OUTPUT],
            rawData[GAS_TEMP_INPUT], rawData[ROOM_TEMP], rawData[ROOM_HUMIDITY],
            rawData[GAS_TARGET_TEMP], isBooted, signalQualityRSSI,
            signalQualityBER);

    if (ind == 9) {
      char l[4];
      itoa(strlen(urlBuffer), l, 10);
      strcpy(urlBuffer, cmds[ind]);
      strcat(urlBuffer, l);
      return urlBuffer;
    }
    isBooted = 0;
    internetSetupNeeded = false;
    return urlBuffer;
  }

  CmdQisFinished reactForSimpleLine() {
    Serial.println();
    // parse SIGNAL QALITY RSSI AND BER values
    if (executedCmdIndex == 7) {
      signalQualityRSSI = sr->lineBuffer[6] - '0';
      int berInd;
      if (sr->lineBuffer[7] != ',') {
        signalQualityRSSI = signalQualityRSSI * 10 + sr->lineBuffer[7] - '0';
        berInd = 9;
      } else {
        berInd = 8;
      }
      signalQualityBER = sr->lineBuffer[berInd] - '0';
      if (sr->lineBuffer[berInd + 1] != '\0') {
        signalQualityBER =
            signalQualityBER * 10 + sr->lineBuffer[berInd + 1] - '0';
      }
      Serial.print(F("signalQualityRSSI "));
      Serial.println(signalQualityRSSI);
      Serial.print(F("signalQualityBER "));
      Serial.println(signalQualityBER);
    }
    return false;
  }
};

#define MAX_DHT_ERROR_BEFORE_REPORT 20
byte dhtErrorOcurredCount = 0;

void measureTemps() {
  Serial.println("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  int tin = (int)(sensors.getTempCByIndex(0) - 0.5); // компенсация погрешности
  rawData[GAS_TEMP_INPUT] = tin;
  Serial.print(tin);
  Serial.print(" *C->[]->");
  int tout = sensors.getTempCByIndex(1);
  rawData[GAS_TEMP_OUTPUT] = tout;
  Serial.print(tout);
  Serial.println(" *C");


  float temperature = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pDHT, &temperature, &humidity, NULL)) !=
      SimpleDHTErrSuccess) {
    Serial.print(F("Read DHT22 failed, err="));
    Serial.println(err);
    dhtErrorOcurredCount++;
    if (dhtErrorOcurredCount > MAX_DHT_ERROR_BEFORE_REPORT) {
      dhtErrorOcurredCount = 0;
      rawData[ROOM_HUMIDITY] = 127;
      rawData[ROOM_TEMP] = 127;
    }
    // else just not update values

  } else {
    rawData[ROOM_HUMIDITY] = humidity;
    rawData[ROOM_TEMP] = temperature;
  }

  Serial.print((float)temperature);
  Serial.print(" *C, ");
  Serial.print((float)humidity);
  Serial.println(" RH%");

  lastMeasureTime = millis();
}

void loop(void) {
  checkExecStatus();
  if (router.available()) {
    executionStatusCode = router.readInput();
  }
  // if (Serial.available())
  //   while (Serial.available()) {
  //     gsmSerial.write(Serial.read());
  //   }
  stepper.run();
  if (needAutomaticallySwitchOffDisplay()) {
    switchOffDisplay();
    return;
  }
  buttonDebouncer.update();
  if (buttonDebouncer.rose()) {
    Serial.println("Rotary button pressed");
    if (getUiState() == DISPLAY_OFF) {
      switchOnDisplay();
      return;
    }
    // if (getUiState() == DISPLAY_ON) {
    //   showMenu();
    //   return;
    // }
    // if (getUiState() == DISPLAY_MENU) {
    //   menuPressed();
    //   return;
    // }
  }
  // backButtonDebouncer.update();
  // if (backButtonDebouncer.rose()) {
  //   // Serial.println("Back button pressed");
  //   if (getUiState() == DISPLAY_OFF) {
  //     switchOnDisplay();
  //     return;
  //   }
  //   if (getUiState() == DISPLAY_ON) {
  //     switchOffDisplay();
  //     return;
  //   }
  //   if (getUiState() == DISPLAY_MENU) {
  //     menuBack();
  //     return;
  //   }
  // }
  rotaryDebouncerA.update();
  if (rotaryDebouncerA.rose()) {
    if (digitalRead(pRotaryB))
      rotaryUp();
    else
      rotaryDown();
  }
  if (timeToMeasure()) {
    measureTemps();
    updateDispay();
  }
  if (stepperRunned)
    disableStepper();
  if (timeToSend()){
    if (router.routerIsBusy())
      return;
    lastSendTime = millis();
    router.executeQ(new DataReportCQ(&router));
  }
}
