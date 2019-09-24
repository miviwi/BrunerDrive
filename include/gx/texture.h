#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

class GLTexture2D {
public:
  struct InvalidFormatTypeError : public std::runtime_error {
    InvalidFormatTypeError() :
      std::runtime_error("invalid format (must be untyped) or format/type combination!")
    { }
  };

  GLTexture2D();
  GLTexture2D(const GLTexture2D&) = delete;
  GLTexture2D(GLTexture2D&& other);
  ~GLTexture2D();

  auto alloc(unsigned width, unsigned height, unsigned levels, GLFormat internalformat) -> GLTexture2D&;
  auto upload(unsigned level, GLFormat format, GLType type, const void *data) -> GLTexture2D&;

  auto id() const -> GLObject;

private:
  GLObject id_;
  unsigned width_, height_;
  unsigned levels_;
};

}
