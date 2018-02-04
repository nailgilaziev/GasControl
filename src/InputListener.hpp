//
//  InputListener.hpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#include "Arduino.h"

#ifndef InputListener_hpp
#define InputListener_hpp

typedef bool CmdQisFinished;

class SerialRouter;

class InputListener {
public:
  InputListener(SerialRouter *sr) : sr(sr) {}
  virtual ~InputListener() {}
  virtual CmdQisFinished newLineEvent(bool isFullLine);

protected:
  SerialRouter *sr;
};

#endif /* InputListener_hpp */
