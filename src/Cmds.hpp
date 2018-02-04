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

#endif /* Cmds_h */
