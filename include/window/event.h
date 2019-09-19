#pragma once

#include <types.h>
#include <window/geometry.h>

#include <exception>
#include <stdexcept>
#include <memory>
#include <deque>

namespace brdrive {

class IEventLoop;

class Event {
public:
  enum Type {
    Invalid,
    Quit,
    KeyDown, KeyUp,
    MouseMove, MouseDown, MouseUp,
  };

  using Ptr = std::unique_ptr<Event>;

  virtual ~Event();

  auto type() const -> Type;

protected:
  Event(Type type) :
    type_(type)
  { }

  Type type_;
};

struct Key {
  enum : u32 {
    LShift, RShift,
    LCtrl, RCtrl,
    LAlt, RAlt,
    LMeta, RMeta,
    Backspace, Tab, Enter,
    Home, End, Insert, Delete,
    PageUp, PageDown,
    Escape,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    PrintScreen, ScrollLock, Pause,
    NumLock, CapsLock,

    Invalid = ~0u,
  };
};

class QuitEvent : public Event {
public:
  QuitEvent();

  static auto alloc() -> Event::Ptr { return Event::Ptr(new QuitEvent()); }
};

class IKeyEvent {
public:
  virtual ~IKeyEvent();

  // Returns the keyboard scancode 
  virtual auto code() -> u32 = 0;

  // Returns the code() converted to an
  //   ASCII character/one of the 'Key'
  //   constants (listed above)
  virtual auto sym() -> u32 = 0;

  // Returns the printable character
  //   i.e. sym() which takes into
  //   account the modifiers held down
  //   at the time of the key press
 // virtual auto character() -> u32 = 0;

protected:
};

class IMouseEvent {
public:
  virtual ~IMouseEvent();

  virtual auto point() -> Vec2<i16> = 0;

  virtual auto delta() -> Vec2<i16> = 0;

protected:
};

class IEventLoop {
public:
  struct InitError : public std::runtime_error {
    InitError() :
      std::runtime_error("failed to initialize the event loop!")
    { }
  };

  enum Flags : u32 {
    Block = (1<<0),
  };

  IEventLoop();
  virtual ~IEventLoop();

  // Must be called before any other method
  auto init() -> IEventLoop&;

  // Returns a nullptr when block == false and there
  //   aren't any events in the queue
  auto event(u32 flags = 0) -> Event::Ptr;

  auto queueEmpty() const -> bool;

protected:
  // Should return 'false' if initialization fails
  virtual auto initInternal() -> bool = 0;

  // Delegate of queueEmpty() when the queue
  //   of IEventLoop (queue_) is empty
  //   which should give the state of the
  //   windowing system's queue
  virtual auto queueEmptyInternal() const -> bool = 0;

  // Should return 'nullptr' if there are no more
  //   events to be processed right now
  virtual auto pollEvent() -> Event::Ptr = 0;

  // Blocking version of pollEvent()
  //   - Must NEVER return 'nullptr'
  virtual auto waitEvent() -> Event::Ptr = 0;

private:
  void fillQueue();

  bool was_init_;
  std::deque<Event::Ptr> queue_;
};

}
