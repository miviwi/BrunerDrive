#pragma once

#include <types.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

using GLEnum   = u16;
using GLObject = unsigned;

enum : GLObject {
  GLNullObject = ~0u,
};

// Can only be called AFTER acquiring an OpenGL context!
void gx_init();
void gx_finalize();

auto gx_was_init() -> bool;

struct GL3WInitError : public std::runtime_error {
  GL3WInitError() :
    std::runtime_error("failed to initialize gl3w!")
  { }
};

}
