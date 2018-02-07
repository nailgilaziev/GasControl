//
//  Cmds.cpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#include "CmdsQueue.hpp"
#include "SerialRouter.hpp"



void CmdsQueue::executeCmd(const char *cmd) {
 Serial.print(F("←"));
  Serial.println(cmd);
  for (int i = 0; true; i++) {
    char c = cmd[i];
    if (c == '\0')
      break;
    sr->s->write(c);
  }
  sr->s->write(CR);
  //sr->s->write(LF);
}

const char *CmdsQueue::getCmd(byte ind) { return NULL; }

CmdQisFinished CmdsQueue::runQ(){
  // When you start you executedCmdIndex = 255
  // and you can read last lineBuffer (response from modem)
  const char *cmd = getCmd(0);
  if (cmd == NULL){
    return true;
  }
  executeCmd(cmd);
  executedCmdIndex++;
  return false;
}

CmdQisFinished CmdsQueue::cmdSuccseed() {
  Serial.println();
  //previous command succeed so launch next command
  const char *cmd = getCmd(executedCmdIndex+1);
  if (cmd == NULL)
    return true;
  executeCmd(cmd);
  executedCmdIndex++;
  return false;
}

CmdQisFinished CmdsQueue::cmdFailed() {
  Serial.println();
  return true;
}


CmdQisFinished CmdsQueue::reactForSimpleLine() {
  Serial.println();
  return false;
}

bool CmdsQueue::responseIs(const char * str, bool exactMatch) {
  if (exactMatch) {
    Serial.print('[');
    Serial.print(str);
    Serial.print(']');
    return strcmp(sr->lineBuffer, str) == 0;
  } else {
    Serial.print('(');
    Serial.print(str);
    Serial.print(')');
    return strstr(sr->lineBuffer, str) != NULL;
  }
}

CmdQisFinished CmdsQueue::newLineEvent(bool isFullLine) {
  if (responseIs("OK")) {
    return cmdSuccseed();
  }
  if (responseIs("ERROR")) {
    return cmdFailed();
  }
  return reactForSimpleLine();
}
