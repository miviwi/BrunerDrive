#pragma once

#include <window/event.h>

namespace brdrive {

using X11ResponseType = u8;
using X11EventHandle  = void /* xcb_generic_event_t */ *;

class X11Event {
public:
  X11Event(const X11Event&) = delete;
  virtual ~X11Event();

protected:
  X11Event(X11EventHandle ev);

  static auto x11_response_type(X11EventHandle ev) -> X11ResponseType;
  static auto type_from_handle(X11EventHandle ev) -> Event::Type;

  X11EventHandle event_;
};

class X11KeyEvent : public X11Event, public IKeyEvent {
public:
  X11KeyEvent(X11EventHandle ev);
  virtual ~X11KeyEvent();

private:
};

}
