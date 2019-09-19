#pragma once

#include <window/event.h>

#include <memory>

namespace brdrive {

using X11ResponseType = u8;
using X11EventHandle  = void /* xcb_generic_event_t */ *;

class X11Event {
public:
  X11Event(const X11Event&) = delete;
  virtual ~X11Event();

  static auto from_X11EventHandle(X11EventHandle ev) -> Event::Ptr;

protected:
  X11Event(X11EventHandle ev);

  static auto x11_response_type(X11EventHandle ev) -> X11ResponseType;
  static auto type_from_handle(X11EventHandle ev) -> Event::Type;

  X11EventHandle event_;
};

class X11KeyEvent : public X11Event, public IKeyEvent {
public:
  virtual ~X11KeyEvent();

private:
  friend X11Event;

  X11KeyEvent(X11EventHandle ev, Event::Type type);
};

class X11MouseEvent : public X11Event, public IMouseEvent {
public:
  virtual ~X11MouseEvent();

private:
  friend X11Event;

  X11MouseEvent(X11EventHandle ev, Event::Type type);
};

}
