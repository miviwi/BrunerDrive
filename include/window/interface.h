#pragma once

#include <window/window.h>
#include <window/event.h>

#include <os/interface.h>

namespace brdrive {

struct WindowImplFactories {
  using NewWindowFn = IWindow *(*)();
  using NewEventLoopFn = IEventLoop *(*)();

  NewWindowFn new_window;
  NewEventLoopFn new_event_loop;
};

using WindowSysInterface = SysInterface<WindowImplFactories>;

}
