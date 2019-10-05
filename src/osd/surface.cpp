#include <osd/surface.h>
#include <osd/font.h>
#include <osd/drawcall.h>

#include <gx/gx.h>
#include <gx/program.h>
#include <gx/texture.h>
#include <gx/buffer.h>

#include <cassert>

namespace brdrive {

// Initialized during osd_init()
GLProgram **OSDSurface::s_surface_programs = nullptr;

OSDSurface::OSDSurface() :
  dimensions_(ivec2::zero()),
  font_(nullptr),
  bg_(Color::transparent()),
  created_(false),
  drawcalls_(nullptr), num_drawcalls_(DrawcallInitialReserve)
{
}

OSDSurface::~OSDSurface()
{
  if(!drawcalls_) return;

  delete[] drawcalls_;
}

auto OSDSurface::create(
    ivec2 width_height, const OSDBitmapFont *font, const Color& bg
  ) -> OSDSurface&
{
  assert((width_height.x > 0) && (width_height.y > 0) &&
      "width and height must be positive integers!");
  assert(s_surface_programs &&
      "osd_init() MUST be called prior to creating any OSDSurfaces!");

  dimensions_ = width_height;
  font_ = font;
  bg_ = bg;

  drawcalls_ = new OSDDrawCall[num_drawcalls_];

  // TODO: also acquire all the prerequisite OpenGL objects here

  created_ = true;

  return *this;
}

auto OSDSurface::writeString(ivec2 pos, const char *string) -> OSDSurface&
{
  assert(string && "attempted to write a nullptr string!");

  // Perform internal state validation
  if(!created_) throw NullSurfaceError();
  if(!font_) throw FontNotProvidedError();

  return *this;
}

auto OSDSurface::renderProgram(int draw_type) -> GLProgram&
{
  assert(s_surface_programs &&
      "attempted to render a surface before calling osd_init()!");
      
  assert((draw_type > OSDDrawCall::DrawTypeInvalid && draw_type < OSDDrawCall::NumDrawTypes) &&
      "the given 'draw_type' is invalid!");

  assert((s_surface_programs[draw_type] && s_surface_programs[draw_type]->linked()) &&
      "attempted to render a surface without calling osd_init()!");

  // Return a GLProgram* or 'nullptr' if the drawcall is a no-op/invalid
  return *s_surface_programs[draw_type];
}

}
