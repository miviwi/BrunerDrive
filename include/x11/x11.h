#pragma once

#include <types.h>

namespace brdrive {

// Forward declaration
class X11Connection;

void x11_init();
void x11_finalize();

auto x11() -> X11Connection&;

}
