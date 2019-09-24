#include <gx/texture.h>
#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <algorithm>
#include <utility>

#include <cassert>

namespace brdrive {

inline auto GLFormat_to_internalformat(GLFormat format) -> GLenum
{
  switch(format) {
  case r:     return GL_RED;
  case rg:    return GL_RG;
  case rgb:   return GL_RGB;
  case rgba:  return GL_RGBA;

  case r8:    return GL_R8;
  case rg8:   return GL_RG8;
  case rgb8:  return GL_RGB8;
  case rgba8: return GL_RGBA8;

  case r16f:  return GL_R16F;
  case rg16f: return GL_RG16F;

  case depth:         return GL_DEPTH_COMPONENT;
  case depth_stencil: return GL_DEPTH_STENCIL;

  case depth16:  return GL_DEPTH_COMPONENT16;
  case depth24:  return GL_DEPTH_COMPONENT24;
  case depth32f: return GL_DEPTH_COMPONENT32F;

  default: ;         // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

inline auto GLFormat_to_format(GLFormat format) -> GLenum
{
  switch(format) {
  case r:    return GL_RED;
  case rg:   return GL_RG;
  case rgb:  return GL_RGB;
  case rgba: return GL_RGBA;

  case depth:         return GL_DEPTH_COMPONENT;
  case depth_stencil: return GL_DEPTH_STENCIL;

  default: ;         // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

inline auto GLType_to_type(GLType type) -> GLenum
{
  switch(type) {
  case GLType::u8:  return GL_UNSIGNED_BYTE;
  case GLType::u16: return GL_UNSIGNED_SHORT;
  case GLType::i8:  return GL_BYTE;
  case GLType::i16: return GL_SHORT;

  case GLType::u16_565:   return GL_UNSIGNED_SHORT_5_6_5;
  case GLType::u16_5551:  return GL_UNSIGNED_SHORT_5_5_5_1;
  case GLType::u16_565r:  return GL_UNSIGNED_SHORT_5_6_5_REV;
  case GLType::u16_1555r: return GL_UNSIGNED_SHORT_1_5_5_5_REV;

  case GLType::f32: return GL_FLOAT;

  default: ;     // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

GLTexture::GLTexture(GLEnum bind_target) :
  id_(GLNullObject),
  bind_target_(bind_target)
{
}

GLTexture::~GLTexture()
{
  if(id_ == GLNullObject) return;

  glDeleteTextures(1, &id_);
}

auto GLTexture::id() const -> GLObject
{
  return id_;
}

auto GLTexture::bindTarget() const -> GLEnum
{
  return GL_TEXTURE_2D;
}

GLTexture2D::GLTexture2D() :
  GLTexture(GL_TEXTURE_2D),
  width_(~0u), height_(~0u)
{
}

GLTexture2D::GLTexture2D(GLTexture2D&& other) :
  GLTexture2D()
{
  std::swap(id_, other.id_);
  std::swap(width_, other.width_);
  std::swap(height_, other.height_);
  std::swap(levels_, other.levels_);
}

auto GLTexture2D::alloc(unsigned width, unsigned height, unsigned levels, GLFormat internalformat) -> GLTexture2D&
{
  // Use direct state access if available
  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glCreateTextures(GL_TEXTURE_2D, 1, &id_);

    glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  } else {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  assert(glGetError() == GL_NO_ERROR);

  // Initialize internal variables
  width_ = width; height_ = height;
  levels_ = levels;

  auto gl_internalformat = GLFormat_to_internalformat(internalformat);
  if(gl_internalformat == GL_INVALID_ENUM) throw InvalidFormatTypeError();

  if(ARB::texture_storage()) {
    glTextureStorage2D(id_, levels, gl_internalformat, width, height);
  } else {
    for(unsigned l = 0; l < levels; l++) {
      glTexImage2D(GL_TEXTURE_2D, l, gl_internalformat, width, height,
        /* border */ 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

      width  = std::max(1u, width/2);
      height = std::max(1u, height/2);
    }
  }

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLTexture2D::upload(unsigned level, GLFormat format, GLType type, const void *data) -> GLTexture2D&
{
  auto gl_format = GLFormat_to_format(format);
  auto gl_type   = GLType_to_type(type);

  if(gl_format == GL_INVALID_ENUM || gl_type == GL_INVALID_ENUM)
    throw InvalidFormatTypeError();

  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glTextureSubImage2D(id_, level, 0, 0, width_, height_, gl_format, gl_type, data);
  } else {
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width_, height_, gl_format, gl_type, data);
  }

  // Check if the provided format/type combination is valid
  auto err = glGetError();
  if(err == GL_INVALID_OPERATION) throw InvalidFormatTypeError();

  // ...and make sure there were no other errors ;)
  assert(err == GL_NO_ERROR);

  return *this;
}

GLSampler::GLSampler() :
  id_(GLNullObject)
{
}

GLSampler::GLSampler(GLSampler&& other) :
  GLSampler()
{
  std::swap(id_, other.id_);
}

GLSampler::~GLSampler()
{
  if(id_ == GLNullObject) return;

  glDeleteSamplers(1, &id_);
}

auto GLSampler::id() const -> GLObject
{
  return id_;
}

GLTexImageUnit::GLTexImageUnit(unsigned slot) :
  slot_(slot),
  bound_texture_(GLNullObject), bound_sampler_(GLNullObject)
{
}

auto GLTexImageUnit::bind(const GLTexture& tex) -> GLTexImageUnit&
{
  assert(tex.id() != GLNullObject && "attempted to bind() a null texture to a GLTexImageUnit!");

  auto tex_id = tex.id();

  // Only bind the texture if it's different than the current one
  if(bound_texture_ == tex_id) return *this;

  glActiveTexture(GL_TEXTURE0 + slot_);
  assert(glGetError() == GL_NO_ERROR);

  glBindTexture(tex.bindTarget(), tex_id);
  bound_texture_ = tex_id;

  return *this;
}

auto GLTexImageUnit::bind(const GLSampler& sampler) -> GLTexImageUnit&
{
  assert(sampler.id() != GLNullObject && "attempted to bind() a null sampler to a GLTexImageUnit!");

  auto sampler_id = sampler.id();

  // Only bind the sampler if it's different than the current one
  if(bound_sampler_ == sampler_id) return *this;

  glBindSampler(slot_, sampler.id());
  bound_sampler_ = sampler.id();

  return *this;
}

auto GLTexImageUnit::bind(const GLTexture& tex, const GLSampler& sampler) -> GLTexImageUnit&
{
  return bind(tex), bind(sampler);
}

}
