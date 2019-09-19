#include <x11/event.h>
#include <x11/connection.h>

#include <xcb/xcb.h>

#include <cassert>
#include <cstdlib>

#include <utility>

namespace brdrive {

template <typename T = xcb_generic_event_t>
static inline auto xcb_ev(X11EventHandle ev) -> const T*
{
  return (T *)ev;
}

X11Event::X11Event(X11EventHandle ev) :
  Event(type_from_handle(ev)),
  event_(ev)
{
}

X11Event::~X11Event()
{
  free(event_);
}

auto X11Event::from_X11EventHandle(X11EventHandle ev) -> Event::Ptr
{
  // Check if 'ev' is a valid handle
  if(!ev) return Event::Ptr();

  auto event = Event::Ptr();
  auto event_type = type_from_handle(ev);

  // Event::Invalid signals that an event which should be squashed
  //   (e.g. XCB_EXPOSE) has occured, so return a nullptr in that case
  if(event_type == Event::Invalid) return Event::Ptr();

  switch(event_type) {
  case Event::KeyUp:
  case Event::KeyDown:
    event.reset(new X11KeyEvent(ev));
    break;
    
  case Event::MouseMove:
  case Event::MouseDown:
  case Event::MouseUp:
    event.reset(new X11MouseEvent(ev));
    break;
  }

  return std::move(event);
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
  X11Event(ev),
  keycode_(Key::Invalid), keysym_(Key::Invalid)
{
  switch(type()) {
  case Event::KeyDown: {
    auto key_press = xcb_ev<xcb_key_press_event_t>(ev);

    keycode_ = key_press->detail;
    keysym_  = x11().keycodeToKeysym(key_press->detail);
    break;
  }

  case Event::KeyUp: {
    auto key_release = xcb_ev<xcb_key_release_event_t>(ev);

    keycode_ = key_release->detail;
    keysym_  = x11().keycodeToKeysym(key_release->detail);
    break;
  }

  default: assert(0);    // Unreachable
  }
}

X11KeyEvent::~X11KeyEvent()
{
}

auto X11KeyEvent::code() -> u32
{
  return keycode_;
}

auto X11KeyEvent::sym() -> u32
{
  return keysym_;
}

X11MouseEvent::X11MouseEvent(X11EventHandle ev) :
  X11Event(ev),
  point_(Vec2<i16>::zero()), delta_(Vec2<i16>::zero())
{
  switch(type()) {
  case Event::MouseMove: {
    auto mouse_move = xcb_ev<xcb_motion_notify_event_t>(ev);

    point_ = { mouse_move->event_x, mouse_move->event_y };
    break;
  }

  case Event::MouseDown: {
    auto mouse_down = xcb_ev<xcb_button_press_event_t>(ev);

    point_ = { mouse_down->event_x, mouse_down->event_y };
    break;
  }

  case Event::MouseUp: {
    auto mouse_up = xcb_ev<xcb_button_release_event_t>(ev);

    point_ = { mouse_up->event_x, mouse_up->event_y };
    break;
  }

  default: assert(0);     // Unreachable
  }
}

X11MouseEvent::~X11MouseEvent()
{
}

auto X11MouseEvent::point() -> Vec2<i16>
{
  return point_;
}

auto X11MouseEvent::delta() -> Vec2<i16>
{
  return delta_;
}

X11EventLoop::~X11EventLoop()
{
}

auto X11EventLoop::initInternal() -> bool
{
  return true;
}

auto X11EventLoop::pollEvent() -> Event::Ptr
{
  auto ev = xcb_poll_for_event(x11().connection<xcb_connection_t>());
  if(!ev) return Event::Ptr();

  // We have obtained a valid event handle but it's still possible
  //   that it points to an internally handled event...
  if(auto event = X11Event::from_X11EventHandle(ev)) return std::move(event);

  // ...and if it does we'll try to poll once more
  return pollEvent();
}

auto X11EventLoop::waitEvent() -> Event::Ptr
{
  auto ev = xcb_wait_for_event(x11().connection<xcb_connection_t>());
  
  // We have obtained a valid event handle but it's still possible
  //   that it points to an internally handled event...
  if(auto event = X11Event::from_X11EventHandle(ev)) return std::move(event);

  // ...and if it does we'll wait some more for a valid one
  ev = xcb_wait_for_event(x11().connection<xcb_connection_t>());
  if(auto event = X11Event::from_X11EventHandle(ev)) return std::move(event);

  // If the event is invalid the SECOND time that means the
  //   application was closed and we should post a QuitEvent
  return QuitEvent::alloc();
}

}
