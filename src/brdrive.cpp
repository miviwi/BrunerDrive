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

  window
    .geometry({ 0, 0, 640, 480 })
    .background(brdrive::Color(1.0f, 0.0f, 1.0f, 0.0f))
    .create()
    .show();

  auto c = brdrive::x11().connection<xcb_connection_t>();

  bool running = true;
  while(auto ev = xcb_wait_for_event(c)) {
    auto event = brdrive::X11Event::from_X11EventHandle(ev);
    if(!event) {
      free(ev);
      continue;
    }

    switch(event->type()) {
    case brdrive::Event::KeyUp: {
      auto key_press = (xcb_key_press_event_t *)ev;

      auto keysym = brdrive::x11().keycodeToKeysym(key_press->detail);

      printf("key_press->detail=(0x%2X %3u, %c) keysym=(0x%2X %3u, %c)\n",
          key_press->detail, key_press->detail, key_press->detail,
          keysym, keysym, keysym);

      if(keysym == 'q') running = false;
      break;
    }

    case brdrive::Event::MouseDown: {
      auto button_press = (xcb_button_press_event_t *)ev;

      printf("click! @ (%hd, %hd)\n", button_press->event_x, button_press->event_y);

      window.drawString("hello, world!",
          brdrive::Geometry::xy(button_press->event_x, button_press->event_y), brdrive::Color::white()); 

      break;
    }
      
    }

    //if(!running) break;
  }

  brdrive::x11_finalize();

  return 0;
}
