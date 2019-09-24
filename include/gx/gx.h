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

enum GLFormat : int {
  r, rg, rgb, rgba,
  r8, rg8, rgb8, rgba8,
  r16f, rg16f,
  depth, depth_stencil,
  depth16, depth24, depth32f,
};

enum class GLType : int {
  u8, u16,
  i8, i16,
  u16_565, u16_5551,
  u16_565r, u16_1555r,
  f16, f32,
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
