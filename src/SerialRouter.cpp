//
//  SerialRouter.cpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#include "SerialRouter.hpp"

bool SerialRouter::routerIsBusy() { return executingQ != NULL; }

void SerialRouter::executeQ(CmdsQueue *q) {
  Serial.println(F("SCQ"));
  executingQ = q;
  lastCmdQSettedTime = millis();
  if(executingQ->runQ()){
    resetExecutingQ();
    Serial.println(F(" <no exec cmd>"));
  }
}

void SerialRouter::resetExecutingQ() {
  if (executingQ) {
    Serial.print(F("RCQ"));
    lastCmdQSettedTime = 0;
    delete executingQ;
    executingQ = NULL;
  }
}

int SerialRouter::available() {
  if (executingQ) {
    if (millis() - lastCmdQSettedTime > OPERATION_TIMEOUT) {
      resetExecutingQ();
      Serial.println(F(" <timeout>"));
    }
  }
  return s->available();
}

bool SerialRouter::executingQInterrupted(bool isFullLine) {
  if (!isFullLine) {
    Serial.println(F("NOT A FULL LINE. INTERRUPTION NOT POSSIBLE"));
    return false;
  }
  if (eventsListener->newLineEvent(isFullLine, true)) { //dryRun
    // Event is presented in events listener - so interrupt execution
    Serial.println();
    resetExecutingQ();
    Serial.println(F(" <interrupt>"));
    return true;
  }
  return false;
}

byte SerialRouter::analyzeLine(bool isFullLine) {
  lineBuffer[lineCursor] = '\0';
  lineCursor = 0;
  Serial.print("→");
  Serial.print(lineBuffer);
  if (executingQ == NULL || executingQInterrupted(isFullLine)){
    Serial.println();
    eventsListener->newLineEvent(isFullLine); //simply process event
  }
  else {
    if (executingQ->newLineEvent(isFullLine)) { /*Cmd Q is Finished */
      resetExecutingQ();
      Serial.println(F(" <CmdQisFinished>"));
      return ROUTER_STATUS_LISTENING_FOR_INPUT;
    }
  }
  return ROUTER_STATUS_WAITING_NEXT_CHUNK_OF_INPUT;
}

RouterStatus SerialRouter::readInput() {
  while (s->available()) {
    if (lineCursor >= LINE_BUFFER_SIZE - 1) {
      return analyzeLine(false);
    }
    char c = s->read();
    if (lineCursor == 0) {
      if (c == CR || c == LF) {
        // skip empty new lines
        continue;
      }
      if (c == '>') {
        lineBuffer[lineCursor] = c;
        lineCursor++;
        return analyzeLine(true);
      }
    }
    if (c == LF) {
      if (lineBuffer[lineCursor - 1] == CR) {
        lineCursor--;
        return analyzeLine(true);
      } else {
        // unpredictable state. This is not impossible
        return ROUTER_STATUS_UNPREDICTABLE_INPUT_END;
      }
    } else {
      lineBuffer[lineCursor++] = c;
    }
  }
  // this means that we read whole input buffer buffer, and not find answer end
  // yet.
  return ROUTER_STATUS_WAITING_NEXT_CHUNK_OF_INPUT;
}
