#include <x11/event.h>

#include <xcb/xcb.h>

#include <cassert>
#include <cstdlib>

namespace brdrive {

template <typename T = xcb_generic_event_t>
static inline auto xcb_ev(X11EventHandle ev) -> const T*
{
  return (T *)ev;
}

X11Event::X11Event(X11EventHandle ev) :
  event_(ev)
{
}

X11Event::~X11Event()
{
  free(event_);
}

auto X11Event::x11_response_type(X11EventHandle ev) -> X11ResponseType
{
  assert(ev);

  return xcb_ev(ev)->response_type & ~0x80;
}

auto X11Event::type_from_handle(X11EventHandle ev) -> Event::Type
{
  assert(ev);

  auto response_type = x11_response_type(ev);
  switch(response_type) {
  case XCB_KEY_PRESS:   return Event::KeyDown;
  case XCB_KEY_RELEASE: return Event::KeyUp;

  case XCB_MOTION_NOTIFY:  return Event::MouseMove;
  case XCB_BUTTON_PRESS:   return Event::MouseDown;
  case XCB_BUTTON_RELEASE: return Event::MouseUp;
  }

  return Event::Invalid;
}

X11KeyEvent::X11KeyEvent(X11EventHandle ev) :
  X11Event(ev), IKeyEvent(type_from_handle(ev))
{
}

X11KeyEvent::~X11KeyEvent()
{
}

}
