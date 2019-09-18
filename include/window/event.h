#pragma once

#include <types.h>

namespace brdrive {

class Event;

class IEventLoop {
public:
  IEventLoop();
  virtual ~IEventLoop();

private:
};

class Event {
public:
  enum Type {
    Invalid,
    KeyDown, KeyUp,
    MouseMove, MouseDown, MouseUp,
  };

  auto type() const -> Type;

protected:
  Event(Type type) :
    type_(type)
  { }

  Type type_;
};

class IKeyEvent : public Event {
public:
  IKeyEvent(Type type);

protected:
};

}
