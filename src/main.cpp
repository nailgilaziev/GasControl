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
#include "SerialRouter.hpp"
#include <AccelStepper.h>
#include "NokiaDisplay.h"
#include <Bounce2.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SimpleDHT.h>
#include <SoftwareSerial.h>

#define WARNING_NOT_STARTED_YET 2

SoftwareSerial gsmSerial(34, 36); // RX, TX

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
const int pButton = A0;
const int pRotaryA = A1;
const int pRotaryB = A2;

Bounce buttonDebouncer = Bounce();
Bounce rotaryDebouncerA = Bounce();
// Bounce rotaryDebouncerB = Bounce();
// not necessary for board with hardware debouncing
// Bounce backButtonDebouncer = Bounce();

// Data from sensors measured by intervals
unsigned long lastMeasureTime = 0;
//3600000
#define MEASURE_INTERVAL 20000

const int pDHT = 3;
SimpleDHT22 dht22;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire devices (not just
// Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
AccelStepper stepper(4, 4, 6, 3, 5);
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

void setup(void) {
  // router.eventsListener.events = NULL;
  // start serial port
  for (int k = 0; k < 8; k++) {
    rawData[k] = 0; //-10 is error/not initialized state
  }
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Started");

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

#define TEMP_STEP 2
#define TEMP_TO_STEP 200

void moveStepper() {
  // stepper.moveTo(temperature * TEMP_TO_STEP);
}

void changeValue(int t) {
  if (rawData[DISPLAY_MODE] == DISPLAY_MENU) {
    rawData[SELECTED_MENU] = constrain(rawData[SELECTED_MENU] + t, 0, 2);
  } else if (rawData[SELECTED_MENU] == SELECTED_MENU_AUTOMATIC) {
    rawData[CLIM_TARGET_TEMP] =
        constrain(rawData[CLIM_TARGET_TEMP] + t, -9, 40);
  } else {
    rawData[GAS_TARGET_TEMP] = constrain(rawData[GAS_TARGET_TEMP] + t, -1,
    90);
  }
  moveStepper();
  updateDispay();
}

void rotaryUp() {
  // Serial.println("+");
  lastInteractionWithUi = millis();
  changeValue(+1);
}

void rotaryDown() {
  // Serial.println("-");
  lastInteractionWithUi = millis();
  changeValue(-1);
}

bool measuredAfterStart = false;
boolean timeToMeasure() {
  if (!measuredAfterStart) {
    if (millis()>40000) {
      measuredAfterStart = true;
      return true;
    }
    return false;
  }
  return millis() - lastMeasureTime > MEASURE_INTERVAL;
}

byte isBooted = 1;

bool internetSetupNeeded = true;

const char *RESPONSE_TCPSETUP_OK = "+TCPSETUP:0,OK";
const char *RESPONSE_TCPSETUP_ERROR = "+TCPSETUP:";
const char *RESPONSE_TCPSEND_OK = ">";
const char *RESPONSE_CONTAINS_ERROR = "error";
const char *RESPONSE_TCPCLOSE_OK = "+TCPCLOSE:0,OK";

class DataReportCQ : public CmdsQueue {

  bool skipSetupProcess;

public:
  DataReportCQ(SerialRouter *sr) : CmdsQueue(sr) {
    skipSetupProcess = !internetSetupNeeded;
  }

  ResponseMatcher successLineForCmd() {
    switch (executingCmdIndex) {
    case 4:
      return (ResponseMatcher){RESPONSE_TCPSETUP_OK, true};
    case 5:
      return (ResponseMatcher){RESPONSE_TCPSEND_OK, true};
    case 7:
      return (ResponseMatcher){RESPONSE_TCPCLOSE_OK, true};
    }
    return CmdsQueue::successLineForCmd();
  }
  ResponseMatcher errorLineForCmd() {
    switch (executingCmdIndex) {
    case 4:
      return (ResponseMatcher){RESPONSE_TCPSETUP_ERROR, false};
    case 5:
      return (ResponseMatcher){RESPONSE_CONTAINS_ERROR, false};
    }
    return CmdsQueue::errorLineForCmd();
  }
  CmdQisFinished reactForSimpleLine() {
    if (executingCmdIndex == 6) {
      return true;
    }
    return false;
  }

  // CmdQisFinished newLineEvent(SerialRouter *sr) override {
  //   if (strstr(sr->lineBuffer, "Error"))
  //     return true;
  //   if (strstr(sr->lineBuffer, "0.0.0.0")) {
  //     internetSetupNeeded = true;
  //     return true;
  //   }
  //   return false;
  // }
  const char *cmds[8] = {"AT+XISP=0",
                         "AT+CGDCONT=1,\"IP\",\"internet.tele2.ru\"",
                         "AT+XIIC=1",
                         "AT+XIIC?",
                         "AT+TCPSETUP=0,184.106.153.149,80",
                         "AT+TCPSEND=0,",
                         "GET /update?api_key=XB08GLN5246NL2K6&headers=false&"
                         "field1=%i&field2=%i&field3=%i&field4=%i&field5=5%i&"
                         "field6=%i HTTP/1.1\r\n"
                         "Host: 184.106.153.149\r\n\r\n",
                         "AT+TCPCLOSE=0"
                       };


 CmdQisFinished cmdFailed() {
   if (executingCmdIndex == 4) {
     executingCmdIndex = 7;
     internetSetupNeeded = true;
     executeCmd(cmds[7]);
   }
   return false;
 }

  char urlBuffer[155];
  const char *getCmd() override {
    if (executingCmdIndex == 0 && skipSetupProcess) {
      executingCmdIndex = 4;
    }
    if (executingCmdIndex >= 7) {
      return NULL;
    }
    if (executingCmdIndex == 3)
      delay(5000); // delay for connection
    // if (executingCmdIndex == 5) {
    //   // FIXME это должно быть не здесь. а в succseed и failure
    //   internetSetupNeeded = false;
    // }
    if (executingCmdIndex != 5 && executingCmdIndex != 6)
      return cmds[executingCmdIndex];

    sprintf(urlBuffer, cmds[6], rawData[GAS_TEMP_OUTPUT],
            rawData[GAS_TEMP_INPUT], rawData[ROOM_TEMP], rawData[ROOM_HUMIDITY],
            rawData[GAS_TARGET_TEMP], isBooted);

    // Serial.print("transformed ");
    // Serial.print(transformed);
    if (executingCmdIndex == 5) {
      char l[4];
      itoa(strlen(urlBuffer), l, 10);
      // Serial.print("<len>");
      // Serial.print(l);
      strcpy(urlBuffer, cmds[executingCmdIndex]);
      strcat(urlBuffer, l);
      return urlBuffer;
    }
    isBooted = 0;
    internetSetupNeeded = false;
    return urlBuffer;
  }
};

void measureTemps() {
  lastMeasureTime = millis();

  // erial.println("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  int tin = (int)sensors.getTempCByIndex(0);
  rawData[GAS_TEMP_INPUT] = tin;
  // Serial.print(tin);
  // Serial.print(" *C->[]->");
  int tout = sensors.getTempCByIndex(1);
  rawData[GAS_TEMP_OUTPUT] = tout;
  Serial.println(tout);
  // Serial.println(" *C");

  float temperature = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pDHT, &temperature, &humidity, NULL)) !=
      SimpleDHTErrSuccess) {
    // Serial.print(F("Read DHT22 failed, err="));
    // Serial.println(err);
    return;
  }
  rawData[ROOM_HUMIDITY] = humidity;
  rawData[ROOM_TEMP] = temperature;

  // Serial.println((float)temperature);
  // // Serial.print(" *C, ");
  // Serial.println((float)humidity);
  // Serial.println(" RH%");

  if (router.routerIsBusy()) return;
  router.executeQ(new DataReportCQ(&router));

}

void loop(void) {
  checkExecStatus();
  if (router.available()) {
    executionStatusCode = router.readInput();

    // Serial.print('%');
  }
  if (Serial.available())
    while (Serial.available()) {
      gsmSerial.write(Serial.read());
    }

  // stepper.run();
  if (needAutomaticallySwitchOffDisplay()) {
    switchOffDisplay();
    return;
  }
  buttonDebouncer.update();
  if (buttonDebouncer.rose()) {
    // Serial.println("Rotary button pressed");
    if (getUiState() == DISPLAY_OFF) {
      switchOnDisplay();
      return;
    }
    if (getUiState() == DISPLAY_ON) {
      showMenu();
      return;
    }
    if (getUiState() == DISPLAY_MENU) {
      menuPressed();
      return;
    }
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
}
