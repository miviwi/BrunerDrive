#include <window/event.h>

namespace brdrive {

IEventLoop::IEventLoop()
{
}

IEventLoop::~IEventLoop()
{
}

Event::~Event()
{
}

auto Event::type() const -> Type
{
  return type_;
}

IKeyEvent::IKeyEvent(Type type) :
  Event(type)
{
}

IKeyEvent::~IKeyEvent()
{
}

IMouseEvent::IMouseEvent(Type type) :
  Event(type)
{
}

IMouseEvent::~IMouseEvent()
{
}

}
