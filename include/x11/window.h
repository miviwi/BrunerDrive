#pragma once

#include <window/window.h>

#include <exception>
#include <stdexcept>

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

  X11Window();
  virtual ~X11Window();

  virtual auto create() -> IWindow&;
  virtual auto show() -> IWindow&;

  auto xcbConnection() -> void *;

private:
  pX11Window *p;
};

}
