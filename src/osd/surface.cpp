#include <osd/surface.h>
#include <osd/font.h>
#include <osd/drawcall.h>

#include <gx/gx.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/texture.h>
#include <gx/buffer.h>

#include <cassert>

#include <utility>

namespace brdrive {

// Initialized during osd_init()
GLProgram **OSDSurface::s_surface_programs = nullptr;

OSDSurface::OSDSurface() :
  dimensions_(ivec2::zero()), font_(nullptr), bg_(Color::transparent()),
  created_(false),
  surface_object_inds_(nullptr), font_tex_(nullptr), font_sampler_(nullptr),
  strings_(nullptr), strings_xy_off_len_(nullptr)
{
}

OSDSurface::~OSDSurface()
{
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

  initGLObjects();

  created_ = true;

  return *this;
}

auto OSDSurface::writeString(ivec2 pos, const char *string, const Color& color) -> OSDSurface&
{
  assert(string && "attempted to write a nullptr string!");

  // Perform internal state validation
  if(!created_) throw NullSurfaceError();
  if(!font_) throw FontNotProvidedError();

  string_objects_.push_back(StringObject {
    pos, std::string(string), color,
  });

  return *this;
}

auto OSDSurface::draw() -> std::vector<OSDDrawCall>
{
  std::vector<OSDDrawCall> drawcalls;

  return std::move(drawcalls);
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

void OSDSurface::initGLObjects()
{
  GLVertexFormat empty_vertex_format;
  empty_vertex_array_ = empty_vertex_format.newVertexArray();

  surface_object_inds_ = new GLIndexBuffer();

  initFontGLObjects();

  strings_ = new GLTextureBuffer(); strings_xy_off_len_ = new GLTextureBuffer();
}

void OSDSurface::initFontGLObjects()
{
  assert(font_);
  font_tex_ = new GLTexture2D(); font_sampler_ = new GLSampler();

  auto& font_tex = *font_tex_;
  auto& font_sampler = *font_sampler_;

  auto num_glyphs = font_->numGlyphs();
  auto glyph_dims = font_->glyphDimensions();
  auto glyph_grid_dimensions = font_->glyphGridLayoutDimensions();

  auto tex_dimensions = ivec2 {
    glyph_grid_dimensions.x * glyph_dims.x,
    glyph_grid_dimensions.y * glyph_dims.y,
  };

  font_tex
    .alloc(tex_dimensions.x, tex_dimensions.y, 1, r8)
    .upload(0, r, GLType::u8, font_->pixelData());

  font_sampler
    .iParam(GLSampler::WrapS, GLSampler::ClampEdge)
    .iParam(GLSampler::WrapT, GLSampler::ClampEdge)
    .iParam(GLSampler::MinFilter, GLSampler::Nearset)
    .iParam(GLSampler::MagFilter, GLSampler::Nearset);
}

void OSDSurface::appendStringDrawcalls()
{
}

}
