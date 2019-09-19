#include <cstdio>
#include <cstdlib>

#include <brdrive.h>
#include <types.h>
#include <window/geometry.h>
#include <window/color.h>
#include <window/window.h>
#include <x11/x11.h>
#include <x11/connection.h>
#include <x11/window.h>
#include <x11/event.h>

#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <unordered_map>

int main(int argc, char *argv[])
{
  brdrive::x11_init();

  brdrive::X11Window window;
  brdrive::X11EventLoop event_loop;

  window
    .geometry({ 0, 0, 640, 480 })
    .background(brdrive::Color(1.0f, 0.0f, 1.0f, 0.0f))
    .create()
    .show();

  event_loop
    .init();

  auto c = brdrive::x11().connection<xcb_connection_t>();

  bool running = true;
  while(auto ev = event_loop.event(brdrive::IEventLoop::Block)) {
    switch(ev->type()) {
    case brdrive::Event::KeyUp: {
      auto event = (brdrive::IKeyEvent *)ev.get();

      auto code = event->code();
      auto sym  = event->sym();

      printf("code=(0x%2X %3u, %c) sym=(0x%2X %3u, %c)\n",
          code, code, code, sym, sym, sym);

      if(sym == 'q') running = false;
      break;
    }

    case brdrive::Event::MouseDown: {
      auto event = (brdrive::IMouseEvent *)ev.get();
      auto point = event->point();

      printf("click! @ (%hd, %hd)\n", point.x, point.y);

      window.drawString("hello, world!",
          brdrive::Geometry::xy(point.x, point.y), brdrive::Color::white()); 

      break;
    }

    case brdrive::Event::Quit:
      running = false;
      break;
    }

    if(!running) break;
  }

  brdrive::x11_finalize();

  return 0;
}
