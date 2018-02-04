//
//  Cmds.hpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#ifndef CmdsQueue_hpp
#define CmdsQueue_hpp

#include "InputListener.hpp"

extern const char* RESPONSE_ERROR;
extern const char* RESPONSE_OK;

struct ResponseMatcher{
  const char * str;
  bool exactMatch;
};

class CmdsQueue : InputListener {
protected:
  byte executingCmdIndex = -1;

public:
  CmdsQueue(SerialRouter *sr):InputListener(sr){ };
  virtual ~CmdsQueue(){};

  void executeCmd(const char*);
  void runQ();
  virtual const char* getCmd();
  virtual ResponseMatcher successLineForCmd();
  virtual ResponseMatcher errorLineForCmd();

  virtual CmdQisFinished newLineEvent(bool) override;
  virtual CmdQisFinished cmdSuccseed();
  virtual CmdQisFinished cmdFailed();
  virtual CmdQisFinished reactForSimpleLine();

};

#endif /* CmdsQueue_hpp */
