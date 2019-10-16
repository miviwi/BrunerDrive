#include <x11/interface.h>

#include <x11/x11.h>
#include <x11/window.h>
#include <x11/event.h>

#include <window/interface.h>
#include <window/window.h>
#include <window/event.h>

using namespace brdrive;

extern "C" {

auto x11_new_window() -> IWindow *;
auto x11_new_event_loop() -> IEventLoop *;

static WindowImplFactories s_x11_impl = {
  .new_window     = x11_new_window,
  .new_event_loop = x11_new_event_loop,
};

void brdriveISys_x11_init()
{
  x11_init();
}

void brdriveISys_x11_finalize()
{
  x11_finalize();
}

auto x11_new_window() -> IWindow *
{
  return new X11Window();
}

auto x11_new_event_loop() -> IEventLoop *
{
  return new X11EventLoop();
}

auto brdriveISys_x11_impl() -> WindowImplFactories *
{
  return &s_x11_impl;
}

}
