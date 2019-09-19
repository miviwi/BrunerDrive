#pragma once

#include <x11/x11.h>
#include <window/event.h>
#include <window/geometry.h>

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

  virtual auto code() -> u32;
  virtual auto sym() -> u32;

private:
  friend X11Event;

  X11KeyEvent(X11EventHandle ev, Event::Type type);

  u32 keycode_;
  u32 keysym_;
};

class X11MouseEvent : public X11Event, public IMouseEvent {
public:
  virtual ~X11MouseEvent();

  virtual auto point() -> Vec2<i16>;
  virtual auto delta() -> Vec2<i16>;

private:
  friend X11Event;

  X11MouseEvent(X11EventHandle ev, Event::Type type);

  Vec2<i16> point_;
  Vec2<i16> delta_;
};

class X11EventLoop : public IEventLoop {
public:
  virtual ~X11EventLoop();

protected:
  virtual auto initInternal() -> bool;

  virtual auto pollEvent() -> Event::Ptr;

  virtual auto waitEvent() -> Event::Ptr;
};

}
