#pragma once

#include <x11/x11.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

using X11ConnectionHandle = void /* xcb_connection_t */ *;
using X11SetupHandle      = const void /* xcb_setup_t */ *;
using X11ScreenHandle     = void /* xcb_screen_t */ *;

// PIMPL struct
struct pX11Connection;

class X11Connection {
public:
  struct X11ConnectError : std::runtime_error {
    X11ConnectError() :
      std::runtime_error("failed to connect to the X server!")
    { }
  };

  X11Connection();
  ~X11Connection();

  auto connect() -> X11Connection&;

  // Returns a xcb_connection_t* as a X11ConnectionHandle
  auto connectionHandle() -> X11ConnectionHandle;
  // Returns a const xcb_setup_t* as a X11SetupHandle
  auto setupHandle() -> X11SetupHandle;
  // Returns a const xcb_sscreen_t* as a X11screenHandle
  auto screenHandle() -> X11ScreenHandle;

  // T must always be xcb_connection_t
  template <typename T>
  auto connection() -> T*
  {
    return (T *)connectionHandle();
  }

  // T must always be xcb_setup_t
  template <typename T>
  auto setup() -> const T*
  {
    return (const T *)setupHandle();
  }

  // T must always be xcb_screen_t
  template <typename T>
  auto screen() -> T*
  {
    return (T *)screenHandle();
  }

  // Returns a new XId siutable for assigning
  //   to new graphics contexts, windows etc.
  auto genId() -> u32;

  // Calls xcb_flush(connection())
  auto flush() -> X11Connection&;

private:
  pX11Connection *p;
};

}
