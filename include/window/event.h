#pragma once

#include <types.h>

#include <memory>

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

  using Ptr = std::unique_ptr<Event>;

  virtual ~Event();

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
  virtual ~IKeyEvent();

protected:
};

class IMouseEvent : public Event {
public:
  IMouseEvent(Type type);
  virtual ~IMouseEvent();

protected:
};

}
