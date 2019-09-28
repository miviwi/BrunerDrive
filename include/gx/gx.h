#pragma once

#include <types.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

using GLEnum   = u16;
using GLObject = unsigned;

using GLSize = int;

enum : GLObject {
  GLNullObject = ~0u,
};

enum GLFormat : int {
  r, rg, rgb, rgba,
  r8, rg8, rgb8, rgba8,
  r16f, rg16f,
  depth,
  depth16, depth24, depth32f,
  depth_stencil,
  depth24_stencil8,
};

enum class GLType : int {
  i8, i16, i32,
  u8, u16, u32,
  u16_565, u16_5551,
  u16_565r, u16_1555r,
  f16, f32, fixed16_16,

  u32_24_8,
  f32_u32_24_8r,
};

static constexpr unsigned GLNumTexImageUnits = 16; // TODO: query OpenGL for this (?)

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
