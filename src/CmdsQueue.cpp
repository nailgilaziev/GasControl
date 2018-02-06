//
//  Cmds.cpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#include "CmdsQueue.hpp"
#include "SerialRouter.hpp"

const char* RESPONSE_ERROR = "ERROR";
const char* RESPONSE_OK = "OK";

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

const char *CmdsQueue::getCmd() { return NULL; }

CmdQisFinished CmdsQueue::runQ(){
  executingCmdIndex = 0;
  const char *cmd = getCmd();
  if (cmd == NULL){
    return true;
  }
  executeCmd(cmd);
  return false;
}

ResponseMatcher CmdsQueue::successLineForCmd() {
  return (ResponseMatcher) {RESPONSE_OK, true};
}

ResponseMatcher CmdsQueue::errorLineForCmd() {
  return (ResponseMatcher) {RESPONSE_ERROR, true};
}

CmdQisFinished CmdsQueue::cmdSuccseed() {
  //previous command succeed so launch next command
  executingCmdIndex++;
  const char *cmd = getCmd();
  if (cmd == NULL)
    return true;
  executeCmd(cmd);
  return false;
}

CmdQisFinished CmdsQueue::cmdFailed() {
  return true;
}

CmdQisFinished CmdsQueue::reactForSimpleLine() {
  return false;
}

bool matched(const char *lineBuffer, ResponseMatcher rule) {
  if (rule.exactMatch) {
    Serial.print('[');
    Serial.print(rule.str);
    Serial.print(']');
    return strcmp(lineBuffer, rule.str) == 0;
  } else {
    Serial.print('(');
    Serial.print(rule.str);
    Serial.print(')');
    return strstr(lineBuffer, rule.str) != NULL;
  }
}

CmdQisFinished CmdsQueue::newLineEvent(bool isFullLine) {
  if (matched(sr->lineBuffer, successLineForCmd())) {
    Serial.println();
    return cmdSuccseed();
  }
  if (matched(sr->lineBuffer, errorLineForCmd())) {
    Serial.println();
    return cmdFailed();
  }
  Serial.println();
  return reactForSimpleLine();
}
