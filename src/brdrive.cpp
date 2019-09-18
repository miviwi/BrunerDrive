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
    switch(ev->response_type & ~0x80) {
    case XCB_EXPOSE: 
      free(ev);
      break;

    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE: {
      auto event = brdrive::X11KeyEvent(ev);
      auto key_press = (xcb_key_press_event_t *)ev;

      auto keysym = brdrive::x11().keycodeToKeysym(key_press->detail);

      printf("key_press->detail=(0x%2X %3u, %c) keysym=(0x%2X %3u, %c)\n",
          key_press->detail, key_press->detail, key_press->detail,
          keysym, keysym, keysym);

      if(keysym == 'q') running = false;
      break;
    }
      /*
    case XCB_EXPOSE:
      xcb_poly_fill_rectangle(c, win, fg, 1, rectangles);
      xcb_image_text_8(c, sizeof(brdrive::Project)-1, win, bg, 45, 55, brdrive::Project);
      xcb_flush(c);
      break;
      */

      /*
    case XCB_KEY_PRESS: {
      auto key_press = (xcb_button_press_event_t *)ev;
      auto keysym = keycode_to_keysym.at(key_press->detail);
      printf("key_press->detail=(0x%2X %3u, %c) keysym=(0x%2X %3u, %c)\n",
          key_press->detail, key_press->detail, key_press->detail,
          keysym, keysym, keysym);

      if(keysym == 'q') running = false;
      break;
    }
    */
    }

    if(!running) break;
  }

  brdrive::x11_finalize();

  return 0;
}
