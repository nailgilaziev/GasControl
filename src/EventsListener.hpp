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


class SerialRouter;

struct EventAction {
  const char *event;
  CmdsQueue *(*actionQ)(SerialRouter*);
  void (*actionFunc)();
};

class EventsListener {
public:
  void newLineEvent(bool);
  EventsListener(SerialRouter *sr) : sr(sr) {}
protected:
  SerialRouter *sr;
};

#endif /* EventsListener_hpp */
