#include <x11/window.h>
#include <x11/connection.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

namespace brdrive {

struct pX11Window {
  xcb_drawable_t window;
  xcb_gcontext_t bg;

  // Returns 'false' on failure
  auto init(const Geometry& geom, const Color& bg_color) -> bool;

private:
  auto createBg(const Color& bg_color) -> bool;
  auto createWindow(const Geometry& geom, const Color& bg_color) -> bool;
};

auto pX11Window::init(const Geometry& geom, const Color& bg_color) -> bool
{
  window = x11().screen<xcb_screen_t>()->root;
  if(!createWindow(geom, bg_color)) return false;

  return true;
}

auto pX11Window::createBg(const Color& bg_color) -> bool
{
  bg = x11().genId();
  u32 mask = XCB_GC_BACKGROUND;
  u32 arg  = bg_color.bgra();
  xcb_create_gc(x11().connection<xcb_connection_t>(), bg, window, mask, &arg);

  return true;
}

auto pX11Window::createWindow(const Geometry& geom, const Color& bg_color) -> bool
{
  auto screen = x11().screen<xcb_screen_t>();

  window = x11().genId();
  u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  u32 args[] = {
    bg_color.bgr(),                       /* XCB_CW_BACK_PIXEL */
    XCB_EVENT_MASK_EXPOSURE               /* XCB_CW_EVENT_MASK */
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
  };
  xcb_create_window(
      x11().connection<xcb_connection_t>(),
      screen->root_depth, window, screen->root, geom.x, geom.y, geom.w, geom.h, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, args
  );

  x11().flush();

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

  xcb_map_window(x11().connection<xcb_connection_t>(), p->window);
  x11().flush();

  return *this;
}

}
