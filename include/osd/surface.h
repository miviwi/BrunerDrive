#pragma once

#include <osd/osd.h>

#include <window/geometry.h>
#include <window/color.h>
#include <gx/handle.h>

#include <exception>
#include <stdexcept>
#include <vector>
#include <memory>

namespace brdrive {

// Forward declarations
class OSDBitmapFont;
class OSDDrawCall;
class GLVertexArray;
class GLProgram;
class GLTexture;
class GLTexture2D;
class GLTextureBuffer;
class GLSampler;
class GLBuffer;
class GLBufferTexture;
class GLIndexBuffer;
class GLPixelBuffer;

// PIMPL struct
struct pOSDSurface;

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

  auto writeString(ivec2 pos, const char *string, const Color& color) -> OSDSurface&;

  auto draw() -> std::vector<OSDDrawCall>;

  static auto renderProgram(int /* OSDDrawCall::DrawType */ draw_type) -> GLProgram&;

private:
  // Functions that manage static class member creation/destruction
  friend void osd_init();
  friend void osd_finalize();

  enum {
    DrawcallInitialReserve = 256,
  };

  void initGLObjects();

  // Call only if create() was called with a font
  void initFontGLObjects();

  void appendStringDrawcalls();

  // Array of GLProgram *[OSDDrawCall::NumDrawTypes]
  //   - NOTE: pointers in this array CAN be nullptr
  //      (done for ease of indexing)
  static GLProgram **s_surface_programs;

  ivec2 dimensions_;
  const OSDBitmapFont *font_;
  Color bg_;

  bool created_;

  // String related data structures
  struct StringObject {
    ivec2 position;
    std::string str;
    Color color;
  };

  std::vector<StringObject> string_objects_;

  // Generic gx objects (used for drawing everything)
  GLVertexArrayHandle empty_vertex_array_;
  GLIndexBuffer *surface_object_inds_;

  // String-related gx objects
  GLTexture2D *font_tex_;
  GLSampler *font_sampler_;

  GLBufferTexture *strings_buf_;
  GLTextureBuffer *strings_tex_;
};

}
