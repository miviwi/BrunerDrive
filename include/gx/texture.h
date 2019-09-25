#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declarations
class GLContext;
class GLTexture;
class GLTexture2D;
class GLSampler;
class GLTexImageUnit;

class GLTexture {
public:
  enum Dimensions {
    DimensionsInvalid,

    TexImage1D,  // GL_TEXTURE_1D
    TexImage2D,  // GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_*
    TexImage3D,  // GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D
  };

  struct InvalidFormatTypeError : public std::runtime_error {
    InvalidFormatTypeError() :
      std::runtime_error("invalid format (must be untyped) or format/type combination!")
    { }
  };

  GLTexture(const GLTexture&) = delete;
  virtual ~GLTexture();

  auto id() const -> GLObject;

  // Returns a value which informs which of the glTextureSubImage*
  //   functions need to be used for this texture
  auto dimensions() const -> Dimensions;
  // Returns the raw GL_TEXTURE_* value to be passed to OpenGL
  //   functions - glBindTexture(), glTexImage()...
  auto bindTarget() const -> GLEnum;

  auto width() const -> unsigned;
  auto height() const -> unsigned;
  auto depth() const -> unsigned;

protected:
  GLTexture(GLEnum bind_target);

  GLObject id_;

  Dimensions dimensions_;
  GLEnum bind_target_;

  // Initialized to 1 by default so only
  //   the appropriate dimensions need to
  //   be set by the concrete texture types
  unsigned width_, height_, depth_;

  unsigned levels_;
};

class GLTexture2D : public GLTexture {
public:
  GLTexture2D();
  GLTexture2D(GLTexture2D&& other);

  auto alloc(unsigned width, unsigned height, unsigned levels, GLFormat internalformat) -> GLTexture2D&;
  auto upload(unsigned level, GLFormat format, GLType type, const void *data) -> GLTexture2D&;
};

class GLSampler {
public:
  GLSampler();
  GLSampler(const GLSampler&) = delete;
  GLSampler(GLSampler&& other);
  ~GLSampler();

  auto id() const -> GLObject;

private:
  GLObject id_;
};

class GLTexImageUnit {
public:
  auto bind(const GLTexture& tex) -> GLTexImageUnit&;
  auto bind(const GLSampler& sampler) -> GLTexImageUnit&;
  auto bind(const GLTexture& tex, const GLSampler& sampler) -> GLTexImageUnit&;

private:
  friend GLContext;

  GLTexImageUnit(unsigned slot);

  unsigned slot_;

  GLObject bound_texture_;
  GLObject bound_sampler_;
};

}
