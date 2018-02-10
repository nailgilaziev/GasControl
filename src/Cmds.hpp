//
//  Cmds.hpp
//  AtGsm
//
//  Created by Nail on 12/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#ifndef Cmds_h
#define Cmds_h

#include <Arduino.h>

#include "CmdsQueue.hpp"
#include "Global.hpp"
#include "ProjectData.h"
#include "SerialRouter.hpp"

extern int8_t rawData[8];

class ConfigureCQ : public CmdsQueue {
public:
  const char *cmds[5] = {"ATE0", "AT+CMGF=1", "AT+CSCS=\"GSM\"",
                         "AT+CNMI=2,2,0,0,0", NULL};
  ConfigureCQ(SerialRouter *sr) : CmdsQueue(sr) {}
  const char *getCmd(byte ind) override { return cmds[ind]; }
};

class RingResetterCQ : public CmdsQueue {
public:
  const char *ringCmds[2] = {"ATH", "AT+CFUN=15"};

  RingResetterCQ(SerialRouter *sr) : CmdsQueue(sr) {}
  const char *getCmd(byte ind) override {
    static unsigned long lastRing = 0;
    static byte ringCount = 0;
    if (millis() - lastRing > 8000) {
      lastRing = millis();
      ringCount = 1;
      return NULL;
    }
    lastRing = millis();
    ringCount++;
    if (ringCount == 4)
      return ringCmds[0];
    if (ind == 1) {
      delay(600); // for user who call
      return ringCmds[1];
    } else
      return NULL;
  }
};

//extern void moveStepper();

void callMoveStepper(const char * cmdForChangePosition){

  int delta = atoi(cmdForChangePosition);
  int8_t nv = rawData[GAS_TARGET_TEMP] + delta;
  rawData[GAS_TARGET_TEMP] = constrain(nv, -1, 90);
  Serial.print("MOVE CMD\ndelta:");
  Serial.println(delta);
  Serial.print("val:");
  Serial.println(rawData[GAS_TARGET_TEMP]);
  moveStepper();
}

class SmsReactorCQ : public CmdsQueue {
public:
  const char *NAIL_PHONE = "+79047612279";
  const char *AYUP_PHONE = "+79061116054";
  const char *SEND_SMS = "AT+CMGS=\"%s\"";
  const char *CONTENT =
      "%s%c%i'C   --> # -->   %c%i'C\ntemperature: %c%i'C (%i%%)\x1A";
  // int cmdForChangePosition = 0;
  char cmdForChangePosition[4] = {'\0'};
  bool isNailPhone = true;
  SmsReactorCQ(SerialRouter *sr) : CmdsQueue(sr) {}

  CmdQisFinished runQ() override {
    if (strstr(sr->lineBuffer, NAIL_PHONE) != NULL) {
      isNailPhone = true;
      return false;
    }
    if (strstr(sr->lineBuffer, AYUP_PHONE) != NULL) {
      isNailPhone = false;
      return false;
    }
    return true;
  }



  CmdQisFinished reactForSimpleLine() override {
    if (executedCmdIndex == 255) {
      if (sr->lineBuffer[0] == '+' || sr->lineBuffer[0] == '-') {
        if (strlen(sr->lineBuffer) > 3)
          return true;
        strcpy(cmdForChangePosition, sr->lineBuffer);
        callMoveStepper(cmdForChangePosition);
        return cmdSuccseed();
      }
      if (strcmp(sr->lineBuffer, "003") == 0) {
        return cmdSuccseed();
      }
    }
    return CmdsQueue::reactForSimpleLine();
  }

  CmdQisFinished newLineEvent(bool isFullLine) override {
    if (executedCmdIndex == 0 && responseIs(">")) {
      return cmdSuccseed();
    }
    return CmdsQueue::newLineEvent(isFullLine);
  }

  char sendBuffer[74];
  const char *getCmd(byte ind) override {
    if (ind == 0) {
      const char *phone;
      if (isNailPhone)
        phone = NAIL_PHONE;
      else
        phone = AYUP_PHONE;
      sprintf(sendBuffer, SEND_SMS, phone);
      return sendBuffer;
    }
    if (ind == 1) {
      char buf[26];
      if (cmdForChangePosition[0] != '\0') {
        strcpy(buf, "command: ");
        strcat(buf, cmdForChangePosition);
        strcat(buf, "; Okay! (");
        char valbuf[3];
        itoa(rawData[GAS_TARGET_TEMP],valbuf,10);
        strcat(buf, valbuf);
        strcat(buf, ")\n");
      } else
        buf[0] = '\0';
      sprintf(sendBuffer, CONTENT, buf, rawData[GAS_TEMP_INPUT] < 0 ? '-' : '+',
              rawData[GAS_TEMP_INPUT], rawData[GAS_TEMP_OUTPUT] < 0 ? '-' : '+',
              rawData[GAS_TEMP_OUTPUT], rawData[ROOM_TEMP] < 0 ? '-' : '+',
              rawData[ROOM_TEMP], rawData[ROOM_HUMIDITY]);
      return sendBuffer;
    }
    return NULL;
  }
};

#endif /* Cmds_h */
