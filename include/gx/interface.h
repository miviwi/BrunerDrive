#pragma once

#include <gx/context.h>

#include <os/interface.h>

namespace brdrive {

class GLImplFactories {
  using NewContextFn = GLContext *(*)();

  NewContextFn new_context();
};

using GLSysInterface = SysInterface<GLImplFactories>;

}
