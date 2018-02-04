//
//  EventsListener.cpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#include "EventsListener.hpp"
#include "Cmds.hpp"
#include "CmdsQueue.hpp"
#include "SerialRouter.hpp"

// CmdsQueue *echoOff() { return new EchoOffCQ(); }
//TODO заменить на ЛЯМБДЫ!!!!!!!
CmdsQueue *configereSms(SerialRouter *sr) { return new ConfigureCQ(sr); }

// CmdsQueue *reactOnSms() { return NULL; }

// ring мы будем игнорировать
//надо слушать 003
//далее мы будем слушать
//смс a25 - установка авто режима и температуры 25 в комнате
//смс m60 - установка ручного режима на котле в 60 градусов
const int eventsCount = 1;
EventAction events[eventsCount] = {
    // {"MODEM:STARTUP", &echoOff, NULL},
    {"+PBREADY", &configereSms, NULL}
    //{"+CMT: \"+79047612279\"", &reactOnSms, NULL},
};


CmdQisFinished EventsListener::newLineEvent(bool isFullLine) {
  for (byte i = 0; i < eventsCount; i++) {
    if (strncmp(events[i].event, sr->lineBuffer, strlen(events[i].event)) ==
        0) {
      sr->resetExecutingQ();
      Serial.println(F("SCQ"));
      if (events[i].actionQ) {
        sr->executeQ(events[i].actionQ(sr));
      }
      if (events[i].actionFunc) {
        events[i].actionFunc();
      }
      break;
    }
  }
  return false;
}
