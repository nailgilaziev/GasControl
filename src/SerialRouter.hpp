//
//  SerialRouter.hpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#ifndef SerialRouter_hpp
#define SerialRouter_hpp

#include "EventsListener.hpp"
#include <SoftwareSerial.h>
#include "LoggerSD.hpp"

#ifndef LINE_BUFFER_SIZE
// maximum is 255 because lineCursor is int8_t type
#define LINE_BUFFER_SIZE 48
#endif

#define CR 13
#define LF 10

typedef int8_t RouterStatus;

// clear state, line_buffer is empty and we listening for input
#define ROUTER_STATUS_LISTENING_FOR_INPUT 0
// we a in a stete of partially read input from Serial and wait a new portion of
// data
#define ROUTER_STATUS_WAITING_NEXT_CHUNK_OF_INPUT 1
// clear state, line_buffer is empty and we listening for input

#define ROUTER_STATUS_ERROR 2

#define ROUTER_STATUS_UNPREDICTABLE_INPUT_END 3

const unsigned long OPERATION_TIMEOUT = 70000;

class SerialRouter {
public:
  SoftwareSerial *s;
  SerialRouter(SoftwareSerial *s) : s(s) {
    eventsListener = new EventsListener(this);
    loggerSD = new LoggerSD();
  };

  char lineBuffer[LINE_BUFFER_SIZE];
  byte lineCursor = 0;

  RouterStatus readInput();
  int available();
  bool routerIsBusy();
  void executeQ(CmdsQueue *q);
  void resetExecutingQ();

private:
  LoggerSD *loggerSD;
  EventsListener *eventsListener;
  unsigned long lastCmdQSettedTime = 0;
  CmdsQueue *executingQ = NULL;
  byte analyzeLine(bool);
  bool executingQInterrupted(bool);
};

#endif /* SerialRouter_hpp */
