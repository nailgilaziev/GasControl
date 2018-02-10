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

// TODO заменить на ЛЯМБДЫ!!!!!!!
CmdsQueue *configereSms(SerialRouter *sr) { return new ConfigureCQ(sr); }

CmdsQueue *ringInterrupter(SerialRouter *sr) { return new RingResetterCQ(sr); }

CmdsQueue *reactOnSms(SerialRouter *sr) { return new SmsReactorCQ(sr); }

// ring мы будем игнорировать
//надо слушать 003
//далее мы будем слушать
//смс a25 - установка авто режима и температуры 25 в комнате
//смс m60 - установка ручного режима на котле в 60 градусов
const int eventsCount = 3;
EventAction events[eventsCount] = {
  {"+PBREADY", &configereSms, NULL},
  {"RING", &ringInterrupter, NULL},
  {"+CMT: \"+79", &reactOnSms, NULL}
                                 };

EventIsPresented EventsListener::newLineEvent(bool isFullLine, bool dryRun) {
  for (byte i = 0; i < eventsCount; i++) {
    if (strncmp(events[i].event, sr->lineBuffer, strlen(events[i].event)) ==
        0) {
      if (dryRun == false) {
        if (events[i].actionQ) {
          sr->executeQ(events[i].actionQ(sr));
        }
        if (events[i].actionFunc) {
          events[i].actionFunc();
        }
      }
      return true;
    }
  }
  return false;
}
