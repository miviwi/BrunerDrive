#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>
#include <array>

namespace brdrive {

struct GLAttrFormat {
  enum AttrType {
    Normalized, UnNormalized,    // glVertexAttribFormat()
    Integer,                     // glVertexAttribIFormat()
  };

  AttrType attr_type;

  unsigned index;
  int size;    // MUST be in the range [1;4]
  GLEnum type; // Stores the OpenGL Glenum representation of the attribute's GLType
  unsigned offset;
};

class GLVertexFormat {
public:
  static constexpr unsigned MaxVertexAttribBindings = 16;
  static constexpr unsigned MaxVertexAttribs = 16;

  using AttrType = GLAttrFormat::AttrType;

  struct InvalidAttribTypeError : public std::runtime_error {
    InvalidAttribTypeError() :
      std::runtime_error("an invalid GLType was passed to [i]attr()!")
    { }
  };

  GLVertexFormat();
  GLVertexFormat(const GLVertexFormat&) = delete;
  GLVertexFormat(GLVertexFormat&& other);
  ~GLVertexFormat();

  // Attribute exposed as single precision (32-bit) floating
  //   point number(s) -> float/vec2/vec3/vec4 in shaders
  //  - The attribute indices are assigned sequantially
  //    starting at 0
  auto attr(
      int size, GLType type, unsigned offset, AttrType attr_type = GLAttrFormat::Normalized
    ) -> GLVertexFormat&;

  // Attribute exposed as (32-bit) integer(s) ->
  //   int/ievc2/ivec3/ivec4 in shaders
  //  - The attribute indices are assigned sequantially
  //    starting at 0
  auto iattr(int size, GLType type, unsigned offset) -> GLVertexFormat&;

private:
  GLObject id_;

  unsigned current_attrib_index_;
  std::array<GLAttrFormat, MaxVertexAttribBindings> attr_formats_;
};

}
