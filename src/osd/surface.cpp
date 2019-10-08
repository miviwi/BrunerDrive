#include <osd/surface.h>
#include <osd/font.h>
#include <osd/drawcall.h>

#include <gx/gx.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/texture.h>
#include <gx/buffer.h>

#include <cassert>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <utility>

namespace brdrive {

// Initialized during osd_init()
GLProgram **OSDSurface::s_surface_programs = nullptr;

OSDSurface::OSDSurface() :
  dimensions_(ivec2::zero()), font_(nullptr), bg_(Color::transparent()),
  created_(false),
  surface_object_inds_(nullptr), font_tex_(nullptr), font_sampler_(nullptr),
  strings_buf_(nullptr), strings_tex_(nullptr)
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

  appendStringDrawcalls(drawcalls);

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

  surface_object_verts_ = new GLVertexBuffer();
  surface_object_inds_ = new GLIndexBuffer();

  surface_object_verts_
    ->alloc(SurfaceVertexBufSize, GLBuffer::StreamDraw, GLBuffer::DynamicStorage);

  /*
  surface_object_inds_
    ->alloc(SurfaceIndexBufSize, GLBuffer::StreamDraw
    */
  

  initFontGLObjects();
}

void OSDSurface::initFontGLObjects()
{
  assert(font_);
  font_tex_ = new GLTexture2D(); font_sampler_ = new GLSampler();
  string_verts_ = new GLVertexBuffer(); string_inds_ = new GLIndexBuffer();
  strings_buf_ = new GLBufferTexture(); strings_tex_ = new GLTextureBuffer();

  string_verts_->alloc(StringVertsGPUBufSize, GLBuffer::StreamDraw, GLBuffer::DynamicStorage);

  // Generate the indices for drawing strings which follow the pattern:
  //    0 1 2 3 0xFFFF 4 5 6 7 0xFFFF 8 9 10 11 0xFFFF...
  std::vector<u16> string_inds;

  string_inds.reserve(NumStringInds);
  for(u16 i = 0; i < NumStringInds; i++) {
    u16 idx = (i % 5) < 4 ? (i % 5)+(i/5)*4 : 0xFFFF;

    string_inds.push_back(idx);
  }

  string_inds_
    ->alloc(StringIndsGPUBufSize, GLBuffer::StaticDraw, string_inds.data());

  GLVertexFormat string_array_format;
  string_array_format
    .iattr(0, 4, GLType::u16, GLVertexFormatAttr::PerInstance)
    .bindVertexBuffer(0, *string_verts_);

  string_array_ = string_array_format.newVertexArray();

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

  strings_buf_->alloc(StringsGPUBufSize, GLBuffer::StreamRead, GLBuffer::MapWrite);
  strings_tex_->buffer(r8ui, *strings_buf_);
}

struct StringInstanceData {
  u16 x, y;
  u16 offset;
  u16 size;
};
static_assert(sizeof(StringInstanceData) == 4*sizeof(u16),
    "StringInstanceData has incorrect layout!");

void OSDSurface::appendStringDrawcalls(std::vector<OSDDrawCall>& drawcalls)
{
  // First - sort all the strings by length so they
  //   can be split into buckets which will then be used
  //   to generate drawcalls
  std::sort(string_objects_.begin(), string_objects_.end(),
      [](const auto& strobj1, const auto& strobj2) { return strobj1.str.size() < strobj2.str.size(); });

#if 0
  puts("Sorted StringObjects:");
  for(const auto& strobj : string_objects_) {
    printf("    (%d, %d) \"%s\"\n", strobj.position.x, strobj.position.y, strobj.str.data());
  }
  puts("");
#endif

  // After sorting:
  //   back().str  -> will always have the largest length
  //   front().str -> will always be the shortest
  const size_t size_amplitude = string_objects_.back().str.size() - string_objects_.front().str.size();
 // printf("size_amplitude: %zu\n", size_amplitude);

  size_t num_buckets = 0;
  if(size_amplitude <= 1) {
    num_buckets = 1;
  } else {
    // Take the log base 2 of the size_amplitude as
    //   the number of buckets
    size_t tmp = size_amplitude;
    while(tmp >>= 1) num_buckets++;
  }
 // printf("num_buckets: %zu\n", num_buckets);

  const size_t strs_per_bucket = ceilf((float)string_objects_.size() / (float)num_buckets);
 // printf("strs_per_bucket: %zu\n", strs_per_bucket);

  std::vector<StringInstanceData> instance_data;
  instance_data.reserve(num_buckets * strs_per_bucket);

  auto strings_buf_mapping = strings_buf_->map(GLBuffer::MapWrite);
  auto strings_buf_ptr = strings_buf_mapping.get<u8>();

  intptr_t strings_buf_offset = 0;
  for(size_t bucket = 0; bucket < num_buckets; bucket++) {
    auto bucket_start = string_objects_.data() + bucket*strs_per_bucket;
    auto bucket_end   = bucket_start + strs_per_bucket-1;

    const size_t bucket_str_size = bucket_end->str.size();

    for(size_t bucket_idx = 0; bucket_idx < strs_per_bucket; bucket_idx++) {
      const auto& bucket_str = *(bucket_start + bucket_idx);
      auto size = bucket_str.str.size();

      instance_data.push_back({
          (u16)bucket_str.position.x, (u16)bucket_str.position.y,
          (u16)strings_buf_offset, (u16)size,
      });

      memcpy(strings_buf_ptr + strings_buf_offset, bucket_str.str.data(), size);
      strings_buf_offset += size;
    }
  }

  string_verts_
    ->upload(instance_data.data());

#if 0
  puts("StringObjects instance_data:");
  for(const auto& id : instance_data) {
    printf("pos: (%hu, %hu), offset: %hu, size: %hu\n", id.x, id.y, id.offset, id.size);
  }
#endif

  drawcalls.push_back(osd_drawcall_strings(
        string_array_.get(), GLType::u16, string_inds_, 0,
        string_objects_.back().str.size(), string_objects_.size(),
        font_tex_, font_sampler_, strings_tex_));

}

}
