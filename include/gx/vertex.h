#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>
#include <array>

namespace brdrive {

// Forward declaration
class GLVertexArray;

struct GLVertexFormatAttr {
  enum Type : u16 {
    Normalized, UnNormalized,    // glVertexAttribFormat()
    Integer,                     // glVertexAttribIFormat()

    AttrInvalid
  };

  Type attr_type = AttrInvalid;

  unsigned buffer_index; // The 'bindingindex' which will be passed to glVertexAttribBinding()
  int size;    // MUST be in the range [1;4]
  GLEnum type; // Stores the OpenGL Glenum representation of the attribute's GLType
  unsigned offset;

  auto attrByteSize() const -> GLSize;
};

class GLVertexFormat {
public:
  static constexpr unsigned MaxVertexBufferBindings = 16;
  static constexpr unsigned MaxVertexAttribs = 16;

  using AttrType = GLVertexFormatAttr::Type;

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

  struct VertexBufferBindingIndexOutOfRangeError : public std::runtime_error {
    VertexBufferBindingIndexOutOfRangeError() :
      std::runtime_error("the values of buffer binding point indices cannot be greater"
        "than GLVertexFormat::MaxVertexBufferBindings")
    { }
  };

  GLVertexFormat();
//  GLVertexFormat(const GLVertexFormat&) = delete;
//  GLVertexFormat(GLVertexFormat&& other);

  // Attribute exposed as single precision (32-bit) floating
  //   point number(s) -> float/vec2/vec3/vec4 in shaders
  //  - The attribute indices are assigned sequantially
  //    starting at 0
  auto attr(
      unsigned buffer_index, int size, GLType type,
      unsigned offset, AttrType attr_type = GLVertexFormatAttr::Normalized
    ) -> GLVertexFormat&;

  // Attribute exposed as (32-bit) integer(s) ->
  //   int/ievc2/ivec3/ivec4 in shaders
  //  - The attribute indices are assigned sequantially
  //    starting at 0
  auto iattr(
      unsigned buffer_index, int size, GLType type, unsigned offset
    ) -> GLVertexFormat&;

  auto createVertexArray() const -> GLVertexArray;

private:
  auto currentAttrSlot() -> GLVertexFormatAttr&;

  // If the currentAttrSlot() (with index current_attrib_index_)
  //   isn't taken the method returns 'current_attrib_index_',
  // However if it IS taken, then the slots are scanned one-by-one
  //   (by incrementing 'current_attrib_index_') and the index
  //   of the first empty slot found is kept in current_attrib_index_
  //   and returned.
  auto nextAttrSlotIndex() -> unsigned;

  // Add the desired vertex attribute at the next free index
  auto appendAttr(
      unsigned buffer_index, int size, GLType type, unsigned offset, AttrType attr_type
    ) -> GLVertexFormat&;

  // Returns 'true' if any call made to [i]attr() matched
  //       attr(buf_binding_index, ...)
  //  i.e. it sets the buffer bound to 'buf_binding_index'
  //       as that attribute's data source
  auto usesVertexBuffer(unsigned buf_binding_index) -> bool;

  unsigned current_attrib_index_;
  std::array<GLVertexFormatAttr, MaxVertexBufferBindings> attributes_;

  // A bitfield which represents vertex buffer indices
  //   which are referenced by the attributes
  //  - The LSB's state represents the allocation state of
  //    'bindingindex' 0, the MSB's - of the 31st
  unsigned vertex_buffer_bitmask_;
};

class GLVertexArray {
public:
  GLVertexArray(const GLVertexArray&) = delete;
  GLVertexArray(GLVertexArray&&);
  ~GLVertexArray();

  auto id() const -> GLObject;

private:
  friend GLVertexFormat;

  // GLVertexArray objects must be acquired through a GLVertexFormat
  //   to facillitate transparent use of 'ARB_vertex_attrib_binding'
  //   on machines which support this extension, or run OpenGL >=4.3
  //  - On older machines the old-style glVertexAttribPointer()
  //    family of functions will be used
  GLVertexArray();

  GLObject id_;
};

}
