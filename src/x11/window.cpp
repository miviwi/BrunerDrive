#include <x11/window.h>
#include <x11/connection.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

#include <array>
#include <unordered_map>
#include <optional>

namespace brdrive {

template <typename... Args>
auto gc_args(Args&&... args)
{
  return std::array<u32, sizeof...(Args)>{ args... };
}

struct pX11Window {
  xcb_connection_t *connection;
  const xcb_setup_t *setup;
  xcb_screen_t *screen;

  xcb_drawable_t window;
  xcb_gcontext_t bg;

  ~pX11Window();

  // Returns 'false' on failure
  auto init(const Geometry& geom, const Color& bg_color) -> bool;

  auto font(const std::string& font_name) -> std::optional<xcb_font_t>
  {
    auto font_it = fonts.find(font_name);
    if(font_it == fonts.end()) {
      // If the font wasn't found try to load it
      auto font = openFont(font_name);
      if(!font) return std::nullopt;   // Failed to open the font

      // Font was opened - add it to 'fonts'
      //   for later retrieval and return it
      fonts.emplace(font_name, font.value());
      return font;
    }

    return font_it->second;
  }

  template <size_t N>
  auto createGc(u32 mask, std::array<u32, N> args) -> std::optional<xcb_gcontext_t>
  {
    auto gc = x11().genId();
    auto gc_cookie = xcb_create_gc_checked(
        connection, gc, window, mask, args.data()
    );
    auto err = xcb_request_check(connection, gc_cookie);
    if(!err) return gc;

    // Error - cleanup return std::nullopt
    free(err);
    return std::nullopt;
  }

private:
  auto createWindow(const Geometry& geom, const Color& bg_color) -> bool;
  auto createBg(const Color& bg_color) -> bool;

  auto openFont(const std::string& font_name) -> std::optional<xcb_font_t>;

  std::unordered_map<std::string, xcb_font_t> fonts;
};

pX11Window::~pX11Window()
{
  xcb_destroy_window(connection, window);
  xcb_free_gc(connection, bg);

  for(const auto& [name, font] : fonts) {
    xcb_close_font(connection, font);
  }
}

auto pX11Window::init(const Geometry& geom, const Color& bg_color) -> bool
{
  connection = x11().connection<xcb_connection_t>();
  setup = x11().setup<xcb_setup_t>();
  screen = x11().screen<xcb_screen_t>();

  window = screen->root;
  if(!createWindow(geom, bg_color)) return false;

  return true;
}

auto pX11Window::openFont(const std::string& font) -> std::optional<xcb_font_t>
{
  auto id = x11().genId();

  auto font_cookie = xcb_open_font_checked(
      connection, id, font.size(), font.data()
  );
  auto err = xcb_request_check(connection, font_cookie);
  if(!err) return id;   // The font was opened successfully

  // There was an error - cleanup and signal failure
  free(err);
  return std::nullopt;
}

auto pX11Window::createWindow(const Geometry& geom, const Color& bg_color) -> bool
{
  window = x11().genId();
  u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  u32 args[] = {
    bg_color.bgr(),                       /* XCB_CW_BACK_PIXEL */
    XCB_EVENT_MASK_EXPOSURE               /* XCB_CW_EVENT_MASK */
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
  };
  auto window_cookie = xcb_create_window_checked(
      connection,
      screen->root_depth, window, screen->root, geom.x, geom.y, geom.w, geom.h, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, args
  );
  auto err = xcb_request_check(connection, window_cookie);
  if(!err) return true;

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

auto pX11Window::createBg(const Color& bg_color) -> bool
{
  if(auto bg_ = createGc(XCB_GC_BACKGROUND, gc_args(bg_color.bgr()))) {
    bg = bg_.value();
    return true;
  }

  return false;
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
  x11().flush();

  return *this;
}

auto X11Window::drawString(const std::string& str, const Geometry& geom, const Color& color,
    const std::string& font_name) -> IWindow&
{
  auto font = p->font(font_name.empty() ? "fixed" : font_name);
  if(!font) throw X11NoSuchFontError();

  auto gc = p->createGc(
    XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT,
    gc_args(color.bgr(), background_.bgr(), font.value())
  );
  if(!gc) throw X11InternalError();
  
  auto draw_cookie = xcb_image_text_8_checked(
      p->connection, str.size(), p->window, gc.value(), geom.x, geom.y, str.data()
  );
  auto err = xcb_request_check(p->connection, draw_cookie);
  if(err) {
    free(err);
    throw X11InternalError();
  }

  // Success => err == nulptr, so no need to cleanup
  xcb_free_gc(p->connection, gc.value());

  return *this;
}

}
