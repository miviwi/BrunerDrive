#pragma once

#include <x11/x11.h>
#include <window/window.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace brdrive {

// PIMPL class
struct pX11Window;

class X11Window : public IWindow {
public:
  struct X11InternalError : public std::runtime_error {
    X11InternalError() :
      std::runtime_error("an error occured during communication with the X server")
    { }
  };

  struct X11NoSuchFontError : public std::runtime_error {
    X11NoSuchFontError() :
      std::runtime_error("the font could not be found!")
    { }
  };

  X11Window();
  X11Window(const X11Window&) = delete;
  X11Window(X11Window&&) = delete;
  virtual ~X11Window();

  virtual auto create() -> IWindow&;
  virtual auto show() -> IWindow&;

  virtual auto drawString(const std::string& str, const Geometry& geom, const Color& color,
      const std::string& font = "") -> IWindow&;

private:
  pX11Window *p;
};

}
