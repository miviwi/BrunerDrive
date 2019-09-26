#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>
#include <array>

namespace brdrive {

struct GLAttrFormat {
  enum AttrType : u16 {
    Normalized, UnNormalized,    // glVertexAttribFormat()
    Integer,                     // glVertexAttribIFormat()

    AttrInvalid
  };

  AttrType attr_type = AttrInvalid;

  unsigned index;
  int size;    // MUST be in the range [1;4]
  GLEnum type; // Stores the OpenGL Glenum representation of the attribute's GLType
  unsigned offset;

  auto attrByteSize() const -> GLSize;
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

  struct ExceededAllowedAttribCountError : public std::runtime_error {
    ExceededAllowedAttribCountError() :
      std::runtime_error("the maximum allowed number (GLVertexFormat::MaxVertexAttribs) of"
          "attributes of a vertex format has been exceeded!")
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
  auto currentAttrSlot() -> GLAttrFormat&;

  // If the currentAttrSlot() (with index current_attrib_index_)
  //   isn't taken the method returns 'current_attrib_index_',
  // However if it IS taken, then the slots are scanned one-by-one
  //   (by incrementing 'current_attrib_index_') and the index
  //   of the first empty slot found is kept in current_attrib_index_
  //   and returned.
  auto nextAttrSlotIndex() -> unsigned;

  // Add the desired vertex attribute at the next free index
  auto appendAttr(
      int size, GLType type, unsigned offset, AttrType attr_type
  ) -> GLVertexFormat&;

  GLObject id_;

  unsigned current_attrib_index_;
  std::array<GLAttrFormat, MaxVertexAttribBindings> attr_formats_;
};

}
