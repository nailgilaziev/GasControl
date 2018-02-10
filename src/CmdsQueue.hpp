//
//  Cmds.hpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#ifndef CmdsQueue_hpp
#define CmdsQueue_hpp

#include <Arduino.h>

typedef bool CmdQisFinished;

extern const char *RESPONSE_ERROR;
extern const char *RESPONSE_OK;

struct ResponseMatcher {
  const char *str;
  bool exactMatch;
};

class SerialRouter;

class CmdsQueue {
public:
  CmdsQueue(SerialRouter *sr) : sr(sr) {}
  virtual ~CmdsQueue(){};

  void executeCmd(const char *);
  virtual CmdQisFinished runQ();
  virtual const char *getCmd(byte);

  virtual CmdQisFinished newLineEvent(bool);
  virtual CmdQisFinished cmdSuccseed();
  virtual CmdQisFinished cmdFailed();
  //сюда могут сыпаться хвосты обрезанных строк, не только с начала isFullLine
  virtual CmdQisFinished reactForSimpleLine();

protected:
  virtual bool responseIs(const char *, bool = true);
  SerialRouter *sr;
  byte executedCmdIndex = 255;
};

#endif /* CmdsQueue_hpp */
