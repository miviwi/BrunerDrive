#pragma once

#include <os/interface.h>

// Forward declaration
namespace brdrive {
class WindowImplFactories;
}

extern "C" {

void brdriveISys_x11_init();
void brdriveISys_x11_finalize();

auto brdriveISys_x11_impl() -> brdrive::WindowImplFactories *;

}
