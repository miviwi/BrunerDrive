#include <window/event.h>

namespace brdrive {

IEventLoop::IEventLoop()
{
}

IEventLoop::~IEventLoop()
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

}
