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


typedef bool EventIsPresented;

class SerialRouter;

struct EventAction {
  const char *event;
  CmdsQueue *(*actionQ)(SerialRouter*);
  void (*actionFunc)();
};

class EventsListener {
public:
  //isFullLine param is ignored
  EventIsPresented newLineEvent(bool isFullLine,bool dryRun = false);
  EventsListener(SerialRouter *sr) : sr(sr) {}
protected:
  SerialRouter *sr;
};

#endif /* EventsListener_hpp */
