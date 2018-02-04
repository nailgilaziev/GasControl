//
//  EventsListener.hpp
//  AtGsm
//
//  Created by Nail on 09/03/2017.
//  Copyright © 2017 Nail. All rights reserved.
//

#ifndef EventsListener_hpp
#define EventsListener_hpp

#include "CmdsQueue.hpp"
#include "InputListener.hpp"

#include "EventsListener.hpp"

class SerialRouter;

struct EventAction {
  const char *event;
  CmdsQueue *(*actionQ)(SerialRouter*);
  void (*actionFunc)();
};

class EventsListener : public InputListener {
public:
  virtual CmdQisFinished newLineEvent(bool) override;
  EventsListener(SerialRouter *sr):InputListener(sr){}
  virtual ~EventsListener(){}
};

#endif /* EventsListener_hpp */
