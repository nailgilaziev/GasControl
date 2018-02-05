//
//  Cmds.hpp
//  AtGsm
//
//  Created by Nail on 12/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#ifndef Cmds_h
#define Cmds_h

#include "CmdsQueue.hpp"
#include <Arduino.h>

// class EchoOffCQ : public CmdsQueue {
//   const char *cmds[1] = {"ATE0"};
//   const char *getNext(int index) override {
//     if (index >= 1)
//       return NULL;
//     return cmds[index];
//   }
// };
const char *cmds[] = {"ATE0","AT+CMGF=1", "AT+CSCS=\"GSM\"",NULL};

class ConfigureCQ : public CmdsQueue {
public:
  ConfigureCQ(SerialRouter *sr):CmdsQueue(sr){}
  const char *getCmd() override {
    return cmds[executingCmdIndex];
  }
};

const char* ringResetCall = "ATH";
unsigned long ringLastRing = 0;
byte ringCount = 0;

class RingResetterCQ : public CmdsQueue {
public:
  RingResetterCQ(SerialRouter *sr):CmdsQueue(sr){}
  const char *getCmd() override {
    if (millis() - ringLastRing > 5000) {
      ringLastRing = millis();
      ringCount = 1;
      return NULL;
    }
    ringCount++;
    if(ringCount==4) return ringResetCall;
    else return NULL;
  }
};
#endif /* Cmds_h */
