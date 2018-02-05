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

const char *ringCmds[2] = {"ATH","AT+CFUN=15"};


class RingResetterCQ : public CmdsQueue {
public:
  RingResetterCQ(SerialRouter *sr):CmdsQueue(sr){}
  const char *getCmd() override {
    static unsigned long lastRing = 0;
    static byte ringCount = 0;
    if (millis() - lastRing > 8000) {
      lastRing = millis();
      ringCount = 1;
      return NULL;
    }
    lastRing = millis();
    ringCount++;
    if(ringCount==4) return ringCmds[0];
    if(executingCmdIndex == 1) {
      delay(600); //for user who call
      return ringCmds[1];
    }
    else return NULL;
  }
};
#endif /* Cmds_h */
