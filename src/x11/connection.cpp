#include <x11/connection.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

#include <unordered_map>

namespace brdrive {

struct pX11Connection {
  xcb_connection_t *connection;
  const xcb_setup_t *setup;
  xcb_screen_t *screen;

  std::unordered_map<xcb_keycode_t, u32> keycode_to_keysym;

  auto keycodeToKeysym(xcb_keycode_t keycode) const -> int
  {
    return keycode_to_keysym.at(keycode);
  }

  auto connect() -> bool;

  auto initKbmap() -> bool;
};

auto pX11Connection::connect() -> bool
{
  connection = xcb_connect(nullptr, nullptr);
  if(!connection) return false;

  setup = xcb_get_setup(connection);
  if(!setup) return false;

  // get the primary (first) screen
  screen = xcb_setup_roots_iterator(setup).data;

  return true;
}

auto pX11Connection::initKbmap() -> bool
{
  auto kbmap_cookie = xcb_get_keyboard_mapping(
      connection,
      setup->min_keycode, setup->max_keycode-setup->min_keycode + 1
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

X11Connection::X11Connection() :
  p(nullptr)
{
}

X11Connection::~X11Connection()
{
  delete p;
}

auto X11Connection::connect() -> X11Connection&
{
  p = new pX11Connection();

  // Establish a connection to the X server
  if(!p->connect()) throw X11ConnectError();

  // Cache commonly used data
  if(!p->initKbmap()) throw X11ConnectError();

  return *this;
}

auto X11Connection::connectionHandle() -> X11ConnectionHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->connection;
}

auto X11Connection::setupHandle() -> X11SetupHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->setup;
}

auto X11Connection::screenHandle() -> X11ScreenHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->screen;
}

auto X11Connection::genId() -> u32
{
  return xcb_generate_id(p->connection);
}

auto X11Connection::flush() -> X11Connection&
{
  xcb_flush(p->connection);

  return *this;
}

auto X11Connection::keycodeToKeysym(X11KeyCode keycode) -> u32
{
  assert(p && "must be called AFTER connect()!");

  return p->keycodeToKeysym(keycode);
}

}
