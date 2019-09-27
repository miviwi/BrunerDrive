#include <gx/vertex.h>
#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <algorithm>
#include <utility>

#include <cassert>

namespace brdrive {

inline constexpr auto GLType_to_type(GLType type) -> GLEnum
{
  switch(type) {
  case GLType::u8:  return GL_UNSIGNED_BYTE;
  case GLType::u16: return GL_UNSIGNED_SHORT;
  case GLType::u32: return GL_UNSIGNED_INT;

  case GLType::i8:  return GL_BYTE;
  case GLType::i16: return GL_SHORT;
  case GLType::i32: return GL_INT;

  case GLType::f16:        return GL_HALF_FLOAT;
  case GLType::f32:        return GL_FLOAT;
  case GLType::fixed16_16: return GL_FIXED;

  default: ;       // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

// Size (in bytes) of a single component of an attibute
//   - NOTE: there ARE however exceptions to this (detailed
//           in the comment to the fuction type_is_packed(),
//           found below)
inline constexpr auto sizeof_type_GLenum(GLEnum type) -> GLSize
{ 
  switch(type) {
  // Integer-valued types
  case GL_BYTE:  // Fallthrough
  case GL_UNSIGNED_BYTE:  return 1;
  case GL_SHORT: // Fallthrough
  case GL_UNSIGNED_SHORT: return 2;
  case GL_INT:   // Fallthrough
  case GL_UNSIGNED_INT:   return 4;

  // Real-valued types
  case GL_HALF_FLOAT:     return 2;
  case GL_FLOAT:          return 4;
  case GL_FIXED:          return 4;

  // Packed integer-valued types
  case GL_INT_2_10_10_10_REV:   // Fallthrough
  case GL_UNSIGNED_INT_2_10_10_10_REV:  return 4;

  // Packed real-valued types
  case GL_UNSIGNED_INT_10F_11F_11F_REV: return 4;

  case GL_DOUBLE: assert(0 && "double precision floats are unsupported!");
  }

  return 0;
}

// When this function returns 'true' for a given attribute's
//   component type then, to calculate the size of the entire
//   attribute the value of sizeof_type_GLenum() must NOT
//   be multiplied by the number of components (stored
//   as GLVertexFormatAttr::size)
inline constexpr auto type_is_packed(GLEnum type) -> bool
{
  switch(type) {
  case GL_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_10F_11F_11F_REV: return true;
  }

  return false;
}

auto GLVertexFormatAttr::attrByteSize() const -> GLSize
{
  if(attr_type == AttrInvalid) return 0;

  auto sizeof_component = sizeof_type_GLenum(type);

  // The attribute's byte size is normally calculated as:
  //     <size of attribute component> * <number of components>
  //   but this is not always the case, because there
  //   exist 'packed' types for the attributes for
  //   which the attribute's size must be calculated as just:
  //           <size of attribute component>
  //   because with these types sizeof_type_GLenum() already
  //   returns the size of the attribute (which is possible
  //   because said packed types have a fixed component count).
  // The conditional expression below makes sure this
  //   special case is accounted for
  auto num_components   = type_is_packed(type) ? 1 : size;

  assert(sizeof_component && "The 'type' of this GLVertexFormatAttr is invalid (?)");

  return num_components * sizeof_component;
}

auto GLVertexFormatAttr::offsetAsPtr() const -> const void *
{
  return (const void *)(uintptr_t)offset;
}

GLVertexFormat::GLVertexFormat() :
  current_attrib_index_(0),
  vertex_buffer_bitmask_(0)
{
}

auto GLVertexFormat::attr(
    unsigned buffer_index, int size, GLType type, unsigned offset, AttrType attr_type
  ) -> GLVertexFormat&
{
  assert((attr_type == AttrType::Normalized) || (attr_type == AttrType::UnNormalized) 
      && "invalid AttryType (GLVertexFormatAttr::Type) passed to attr()!");

  return appendAttr(buffer_index, size, type, offset, attr_type);
}

auto GLVertexFormat::iattr(
    unsigned buffer_index, int size, GLType type, unsigned offset
  ) -> GLVertexFormat&
{
   return appendAttr(buffer_index, size, type, offset, AttrType::Integer);
}

auto GLVertexFormat::createVertexArray() const -> GLVertexArray
{
  // Use the ARB_vertex_attrib_binding extension path if it's available...
  if(ARB::vertex_attrib_binding()) return createVertexArray_vertex_attrib_binding();

  // ...and if not - a fallback (pure ARB_vertex_array_object) one
  return createVertexArray_vertex_array_object();
}

auto GLVertexFormat::currentAttrSlot() -> GLVertexFormatAttr&
{
  return attributes_.at(current_attrib_index_);
}

auto GLVertexFormat::nextAttrSlotIndex() -> unsigned
{
  // First check if the currentAttrSlot() is free...
  if(currentAttrSlot().attr_type == AttrType::AttrInvalid) return current_attrib_index_;

  // ...and if not we must search for one
  do {
    current_attrib_index_++;

    // Make sure we don't move past the end of the 'attributes_' array
    if(current_attrib_index_ >= MaxVertexAttribs) throw ExceededAllowedAttribCountError();
  } while(currentAttrSlot().attr_type != AttrType::AttrInvalid);

  return current_attrib_index_;
}

auto GLVertexFormat::appendAttr(
    unsigned buffer_index, int size, GLType type, unsigned offset, AttrType attr_type
  ) -> GLVertexFormat&
{
  // Find a free attribute slot index
  auto attr_slot_idx = nextAttrSlotIndex();
  auto& attr_slot = attributes_.at(attr_slot_idx);

  // Convert 'type_' to an OpenGL GLenum value
  auto gl_type = GLType_to_type(type);
  if(gl_type == GL_INVALID_ENUM) throw InvalidAttribTypeError();

  // Ensure 'buffer_index' is in the allowable range
  if(buffer_index >= MaxVertexBufferBindings) throw VertexBufferBindingIndexOutOfRangeError();

  // Save the attribute's properties to an internal data structure
  attr_slot = GLVertexFormatAttr {
    attr_type,

    buffer_index,
    size, gl_type, offset,
  };

  // Mark the bit corresponding to 'buffer_index' in
  //   the 'vertex_buffer_bitmask_' so this index will
  //   be considered used/required by the vertex format
  vertex_buffer_bitmask_ |= 1u << buffer_index;

  // Advance the current attribute index to the next slot
  current_attrib_index_++;

  return *this;
}

auto GLVertexFormat::usesVertexBuffer(unsigned buf_binding_index) -> bool
{
  assert(buf_binding_index < MaxVertexBufferBindings &&
    "the 'buf_binding_index' must be in the range [0;MaxVertexBufferBindings]");

  auto m = vertex_buffer_bitmask_;
  return (m >> buf_binding_index) & 1;
}

// Given the name of an OpenGL VertexArray and a description
//   of the format - createVertexArrayGeneric_impl() initializes
//   the OpenGL-side data of the VertexArray according to the
//   passed description. Does NOT do error checking, as it's
//   expected to already have been done earlier (i.e. the funct-
//   -ion expects sane input data)
// NOTE: The function also takes into account if either one of
//       <ARB,EXT>_direct_state_access are available (runtime
//       check) and uses optimized code paths
// NOTE: This function does NOT create the vertex array, that
//       is the responsibility of the caller
template <vertex_format_detail::CreateVertexArrayPath CreatePath>
inline void createVertexArrayGeneric_impl(
    GLObject arrayid,
    const std::array<GLVertexFormatAttr, GLVertexFormat::MaxVertexAttribs>& attribs
  )
{
  const auto direct_state_access = ARB::direct_state_access() || EXT::direct_state_access();

  // The direct state access path can ONLY be used if one of the <ARB,EXT>_direct_state_access
  //   extensions is available AND we're using the Path_vertex_attrib_binding execution path,
  //   as the Path_vertex_array_object path requires calling functions which have no DSA
  //   version and so - the vertex array would have to get bound anyways then, which would
  //   dwarf all the performance advantage of DSA
  const auto dsa_path /* direct state access path */ =
    direct_state_access && CreatePath == vertex_format_detail::Path_vertex_attrib_binding;

  // The format_vertex_attrib_binding, format_vertex_array_object
  //   lambdas exist to make the attribute iteration loop tidier

  const auto format_vertex_attrib_binding = [=](
      size_t attr_idx, const GLVertexFormatAttr& attr
  ) {
    // Setup the attribute's format
    if(attr.attr_type == GLVertexFormatAttr::Integer) {
      if(dsa_path) {
        glVertexArrayAttribIFormat(arrayid, attr_idx, attr.size, attr.type, attr.offset);
      } else {
        glVertexAttribIFormat(attr_idx, attr.size, attr.type, attr.offset);
      }
    } else {
      // Check if the (non-floating point) values should be normalized
      GLboolean normalized = (attr.attr_type == GLVertexFormatAttr::Normalized) ? GL_TRUE : GL_FALSE;

      if(dsa_path) {
        glVertexArrayAttribFormat(arrayid, attr_idx, attr.size, attr.type, normalized, attr.offset);
      } else {
        glVertexAttribFormat(attr_idx, attr.size, attr.type, normalized, attr.offset);
      }
    }

    // Set the binding point to where the vertex buffer which holds the data is bound
    if constexpr(CreatePath == vertex_format_detail::Path_vertex_attrib_binding) {
      if(dsa_path) {
        glVertexArrayAttribBinding(arrayid, attr_idx, attr.buffer_index);
      } else {
        glVertexAttribBinding(attr_idx, attr.buffer_index);
      }
    }
  };

  const auto format_vertex_array_object = [=](
      size_t attr_idx, const GLVertexFormatAttr& attr
  ) {
    // Setup the attribute's format
    if(attr.attr_type == GLVertexFormatAttr::Integer) {
      glVertexAttribIPointer(
        attr_idx, attr.size, attr.type, 0 /* TODO: pass proper stride here */,
        attr.offsetAsPtr() /* TODO: attr.offset is relative, need to convert it to a cummulative one */);
    } else {
      // Check if the (non-floating point) values should be normalized
      GLboolean normalized = (attr.attr_type == GLVertexFormatAttr::Normalized) ? GL_TRUE : GL_FALSE;

      glVertexAttribPointer(attr_idx, attr.size, attr.type, normalized,
        0, /* TODO: same as above - pass proper stride */
        attr.offsetAsPtr() /* TODO: use cummulative offset instead of the stored (i.e. relative) one */);
    }
  };

  // ------------------------------------------------------------

  if(!dsa_path) glBindVertexArray(arrayid);

  for(size_t attr_idx = 0; attr_idx < GLVertexFormat::MaxVertexAttribs; attr_idx++) {
    const auto& attr = attribs.at(attr_idx);

    // Don't process empty (unused) attribute slots
    if(attr.attr_type == GLVertexFormatAttr::AttrInvalid) continue;

    // Found a used attribute slot!
    //   - Enable it in the VertexArray
    if(dsa_path) {
      glEnableVertexArrayAttrib(arrayid, attr_idx);
    } else {
      glEnableVertexAttribArray(attr_idx);
    }

    // - Setup it's format
    //  -> Using a compile-time if statement we reduce code repetition AND slightly
    //     improve performance, whereas all other methods would sacrifice one or the
    //     other. Thanks C++17 :)
    //  -> The format_vertex_* functions are lambdas defined above the loop
    if constexpr(CreatePath == vertex_format_detail::Path_vertex_attrib_binding) {
      format_vertex_attrib_binding(attr_idx, attr);
    } else if(CreatePath == vertex_format_detail::Path_vertex_array_object) {
      format_vertex_array_object(attr_idx, attr);
    }

    // Make sure no error(s) have occured
    assert(glGetError() == GL_NO_ERROR);
  }

  // Unbind the VAO from the context.
  //   - They record global state, so subsequent
  //     seemingly unrelated function calls could
  //     leave it in an altered state
  if(!dsa_path) glBindVertexArray(0);
}

auto GLVertexFormat::createVertexArray_vertex_attrib_binding() const -> GLVertexArray
{
  GLVertexArray array;
  glGenVertexArrays(1, &array.id_);

  GLObject arrayid = array.id_;

  createVertexArrayGeneric_impl<vertex_format_detail::Path_vertex_attrib_binding>(
      arrayid, attributes_
  );
    
  return std::move(array);

}

auto GLVertexFormat::createVertexArray_vertex_array_object() const -> GLVertexArray
{
  GLVertexArray array;
  glGenVertexArrays(1, &array.id_);

  GLObject arrayid = array.id_;

  createVertexArrayGeneric_impl<vertex_format_detail::Path_vertex_array_object>(
      arrayid, attributes_
  );
    
  return std::move(array);
}

GLVertexArray::GLVertexArray() :
  id_(GLNullObject)
{
}

GLVertexArray::GLVertexArray(GLVertexArray&& other) :
  GLVertexArray()
{
  std::swap(id_, other.id_);
}

GLVertexArray::~GLVertexArray()
{
  if(id_ == GLNullObject) return;

  glDeleteVertexArrays(1, &id_);
}

auto GLVertexArray::id() const -> GLObject
{
  return id_;
}

auto GLVertexArray::bind() -> GLVertexArray&
{
  assert(id_ != GLNullObject && "attempted to bind() a null GLVertexArray!");

  glBindVertexArray(id_);

  return *this;
}

}
