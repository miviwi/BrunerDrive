#include <x11/window.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

#include <unordered_map>

namespace brdrive {

struct pX11Window {
  xcb_connection_t *connection;
  const xcb_setup_t *setup;
  xcb_screen_t *screen;

  xcb_drawable_t window;
  xcb_gcontext_t bg;

  std::unordered_map<xcb_button_t, int> keycode_to_keysym;

  auto keycodeToKeysym(xcb_button_t keycode) const -> int
  {
    return keycode_to_keysym.at(keycode);
  }

  // Returns a new XId siutable for assigning
  //   to new graphics contexts, windows etc.
  auto genId() -> u32;

  // Returns 'false' on failure
  auto init(const Geometry& geom, const Color& bg_color) -> bool;

  // Calls xcb_flush(connection)
  void flush();

private:
  auto initKbmap() -> bool;

  auto createBg(const Color& bg_color) -> bool;
  auto createWindow(const Geometry& geom, const Color& bg_color) -> bool;
};

auto pX11Window::genId() -> u32
{
  return xcb_generate_id(connection);
}

auto pX11Window::init(const Geometry& geom, const Color& bg_color) -> bool
{
  connection = xcb_connect(nullptr, nullptr);
  if(!connection) return false;

  setup = xcb_get_setup(connection);
  if(!setup) return false;

  // Initialize the keycode_to_keysym std::unordered_map
  if(!initKbmap()) return false;

  // Get the primary (first) screen
  screen = xcb_setup_roots_iterator(setup).data;
  window = screen->root;

  if(!createWindow(geom, bg_color)) return false;

  return true;
}

void pX11Window::flush()
{
  xcb_flush(connection);
}

auto pX11Window::initKbmap() -> bool
{
  auto kbmap_cookie = xcb_get_keyboard_mapping(
      connection,
      setup->min_keycode, setup->max_keycode+setup->min_keycode + 1
  );
  auto kbmap = xcb_get_keyboard_mapping_reply(
      connection, kbmap_cookie, nullptr
  );
  if(!kbmap) return false;

  auto num_keysyms = kbmap->length;
  auto keysyms_per_keycode = kbmap->keysyms_per_keycode;
  auto num_keycodes = num_keysyms / keysyms_per_keycode;

  auto keysyms = (xcb_keysym_t *)(kbmap + 1);

  for(int keycode_idx = 0; keycode_idx < num_keycodes; keycode_idx++) {
    auto keycode = setup->min_keycode + keycode_idx;
    auto lowercase_keysym_idx = keycode_idx*keysyms_per_keycode + 0;

    keycode_to_keysym.emplace(keycode, keysyms[lowercase_keysym_idx]);
  }

  free(kbmap);

  return true;
}

auto pX11Window::createBg(const Color& bg_color) -> bool
{
  bg = genId();
  u32 mask = XCB_GC_BACKGROUND;
  u32 arg  = bg_color.bgra();
  xcb_create_gc(connection, bg, window, mask, &arg);

  return true;
}

auto pX11Window::createWindow(const Geometry& geom, const Color& bg_color) -> bool
{
  window = genId();
  u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  u32 args[] = {
    bg_color.bgr(),                       /* XCB_CW_BACK_PIXEL */
    XCB_EVENT_MASK_EXPOSURE               /* XCB_CW_EVENT_MASK */
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
  };
  xcb_create_window(
      connection,
      XCB_COPY_FROM_PARENT, window, screen->root, geom.x, geom.y, geom.w, geom.h, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, mask, args
  );

  flush();

  return true;
}

X11Window::X11Window() :
  p(nullptr)
{
}

X11Window::~X11Window()
{
  delete p;
}

auto X11Window::create() -> IWindow&
{
  p = new pX11Window();

  if(!p->init(geometry_, background_)) throw X11InternalError();

  return *this;
}

auto X11Window::show() -> IWindow&
{
  assert(p && "can only be called after create()!");

  xcb_map_window(p->connection, p->window);
  p->flush();

  return *this;
}

auto X11Window::xcbConnection() -> void *
{
  assert(p && "can only be called after create()!");

  return p->connection;
}

}
