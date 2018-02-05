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

extern const char* RESPONSE_ERROR;
extern const char* RESPONSE_OK;

struct ResponseMatcher{
  const char * str;
  bool exactMatch;
};

class SerialRouter;

class CmdsQueue  {
public:
  CmdsQueue(SerialRouter *sr): sr(sr){ }
  virtual ~CmdsQueue(){};

  void executeCmd(const char*);
  CmdQisFinished runQ();
  virtual const char* getCmd();
  virtual ResponseMatcher successLineForCmd();
  virtual ResponseMatcher errorLineForCmd();

  virtual CmdQisFinished newLineEvent(bool);
  virtual CmdQisFinished cmdSuccseed();
  virtual CmdQisFinished cmdFailed();
  virtual CmdQisFinished reactForSimpleLine();

protected:
  SerialRouter *sr;
  byte executingCmdIndex = -1;
};

#endif /* CmdsQueue_hpp */
