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
  struct InvalidFormatTypeError : public std::runtime_error {
    InvalidFormatTypeError() :
      std::runtime_error("invalid format (must be untyped) or format/type combination!")
    { }
  };

  GLTexture(const GLTexture&) = delete;
  virtual ~GLTexture();

  auto id() const -> GLObject;
  auto bindTarget() const -> GLEnum;

protected:
  GLTexture(GLEnum bind_target);

  GLObject id_;
  GLEnum bind_target_;
};

class GLTexture2D : public GLTexture {
public:
  GLTexture2D();
  GLTexture2D(GLTexture2D&& other);

  auto alloc(unsigned width, unsigned height, unsigned levels, GLFormat internalformat) -> GLTexture2D&;
  auto upload(unsigned level, GLFormat format, GLType type, const void *data) -> GLTexture2D&;

private:
  unsigned width_, height_;
  unsigned levels_;
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
