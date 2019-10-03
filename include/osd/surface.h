#pragma once

#include <osd/osd.h>
#include <window/geometry.h>
#include <window/color.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declarations
class OSDBitmapFont;
class OSDDrawCall;
class GLProgram;
class GLTexture;
class GLTexture2D;
class GLTextureBuffer;
class GLBuffer;
class GLBufferTexture;
class GLPixelBuffer;

using ivec2 = Vec2<int>;

class OSDSurface {
public:
  struct NullSurfaceError : public std::runtime_error {
    NullSurfaceError() :
      std::runtime_error("create() wasn't called!")
    { }
  };

  struct FontNotProvidedError : public std::runtime_error {
    FontNotProvidedError() :
      std::runtime_error("a font wasn't provided to create()")
    { }
  };

  OSDSurface();
  ~OSDSurface();

  auto create(
      ivec2 width_height, const OSDBitmapFont *font = nullptr,
      const Color& bg = Color::transparent()
  ) -> OSDSurface&;

  auto writeString(ivec2 pos, const char *string) -> OSDSurface&;

private:
  // Functions that manage static class member creation/destruction
  friend void osd_init();
  friend void osd_finalize();

  static GLProgram *s_surface_program;

  ivec2 dimensions_;
  const OSDBitmapFont *font_;
  Color bg_;

  bool created_;
};

}
