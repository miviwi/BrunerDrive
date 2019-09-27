#include <gx/vertex.h>
#include <gx/extensions.h>
#include <gx/buffer.h>

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
  auto num_components_ = type_is_packed(type) ? 1 : num_components;

  assert(sizeof_component && "The 'type' of this GLVertexFormatAttr is invalid (?)");

  return num_components_ * sizeof_component;
}

auto GLVertexFormatAttr::offsetAsPtr() const -> const void *
{
  return (const void *)(uintptr_t)offset;
}
// NOTE: All non-floating point attributes have no concept
//       of being normalized, thus the function's return
 //      value is undefined for such attributes
auto GLVertexFormatAttr::normalized() const -> bool
{
  return (attr_type == Normalized);
}

GLVertexFormat::GLVertexFormat() :
  current_attrib_index_(0),
  vertex_buffer_bitmask_(0),
  padding_bytes_(0),
  cached_vertex_size_(std::nullopt)
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

auto GLVertexFormat::padding(GLSize padding_bytes) -> GLVertexFormat&
{
  assert((padding_bytes >= 0 && padding_bytes <= MaxVertexAttribStride) &&
      "padding the vertex with the requested size would cause it to"
      " exceed to max allowed size for a vertex!");

  padding_bytes_ = padding_bytes;

  // Make sure the vertexByteSize() gets recalculated upon next query
  invalidateCachedVertexSize(); 

  return *this;
}

auto GLVertexFormat::vertexByteSize() const -> GLSize
{
  // First, check if there is a cached value available...
  if(cached_vertex_size_) return cached_vertex_size_.value();    // There is!

  // Accumulates the size of all the attributes
  GLSize accumulator = 0;

  // The loop below assumes attributes with larger
  //   indices come later in memory, so enure this
  //   is the case
  auto sorted_attrs = AttributeArray();       // Make a deep copy of 'attributes_'
  std::copy(attributes_.cbegin(), attributes_.cend(), sorted_attrs.begin());

  // Sort by offset in ascending order
  std::sort(sorted_attrs.begin(), sorted_attrs.end(),
      [](const auto& a, const auto& b) -> bool { return a.offset < b.offset; }
  );

  for(const auto& attr : sorted_attrs) {
    // Skip empty attributes
    if(attr.attr_type == AttrType::AttrInvalid) continue;

    // The size of an attribute must be computed like so
    //   in order to respect the (possibly) introduced padding:
    //  - compute the attribute's size via attrByteSize()
    //  - check if the accumulator's current value matches
    //    the attribute's desired offset - if not, this
    //    means the last one was padded, so add:
    //        <expected offset> - accumulator
    //    to account for the padding
    auto attrib_size = attr.attrByteSize();
    auto attrib_pad  = std::max((GLSize)attr.offset - accumulator, 0);
    accumulator += attrib_pad /* account for the LAST attribute's padding */ + attrib_size;
  }

  // Make sure to account for the padding of the last attribute
  //   - See comment above the declaration of padding()
  GLSize vertex_size = accumulator + padding_bytes_;

  // Cache the result to spped up future queries
  cached_vertex_size_ = vertex_size;

  return vertex_size;
}

auto GLVertexFormat::createVertexArray() const -> GLVertexArray
{
  // Perform some validation...
  if(vertexByteSize() > MaxVertexSize) throw VertexExceedesMaxSizeError();

  // ...and actually create the array:
  //   - Use the ARB_vertex_attrib_binding extension path if it's available...
  if(ARB::vertex_attrib_binding()) return createVertexArray_vertex_attrib_binding();

  //   - ...and if not - a fallback (pure ARB_vertex_array_object) one
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

   // Ensure 'buffer_index' is in the allowable range...
  if(buffer_index >= MaxVertexBufferBindings) throw VertexBufferBindingIndexOutOfRangeError();

  // ...as well as the 'size' (number of components)...
  if(size < 1 || size > 4) throw InvalidNumberOfComponentsError();

  // ...and the offset as well
  if(offset >= MaxVertexAttribRelativeOffset) throw VerterAttribOffsetOutOfRangeError();

  // Convert 'type_' to an OpenGL GLenum value
  auto gl_type = GLType_to_type(type);
  if(gl_type == GL_INVALID_ENUM) throw InvalidAttribTypeError();

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

  // Make sure the vertexByteSize() gets recalculated upon next query
  invalidateCachedVertexSize(); 

  return *this;
}

auto GLVertexFormat::usesVertexBuffer(unsigned buf_binding_index) -> bool
{
  assert(buf_binding_index < MaxVertexBufferBindings &&
    "the 'buf_binding_index' must be in the range [0;MaxVertexBufferBindings]");

  auto m = vertex_buffer_bitmask_;
  return (m >> buf_binding_index) & 1;
}

// Given the description of the vertex format's attributes -
//   createVertexArrayGeneric_impl() creates an OpenGL VertexArray
//   and fills the OpenGL-side data of the VertexArray according
//   to the passed description. Does NOT do error checking, as it's
//   expected to already have been done earlier (i.e. the funct-
//   -ion expects sane input data)
// - Returns a GLObject holding the name of the created VertexArray
//
// NOTE: The function also takes into account if either one of
//       <ARB,EXT>_direct_state_access are available (runtime
//       check) and uses optimized code paths
template <vertex_format_detail::CreateVertexArrayPath CreatePath>
inline auto createVertexArrayGeneric_impl(
    const std::array<GLVertexFormatAttr, GLVertexFormat::MaxVertexAttribs>& attribs
  ) -> GLObject
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
      GLObject vertex_array, size_t attr_idx, const GLVertexFormatAttr& attr
  ) {
    // Setup the attribute's format
    if(attr.attr_type == GLVertexFormatAttr::Integer) {
      if(dsa_path) {
        glVertexArrayAttribIFormat(
            vertex_array, attr_idx, attr.num_components,
            attr.type, attr.offset
        );
      } else {
        glVertexAttribIFormat(attr_idx, attr.num_components, attr.type, attr.offset);
      }
    } else {
      // Check if the non-floating point data should be normalized
      //   when it's converted to floats
      GLboolean normalized = attr.normalized();

      if(dsa_path) {
        glVertexArrayAttribFormat(
            vertex_array, attr_idx, attr.num_components, attr.type,
            normalized, attr.offset
        );
      } else {
        glVertexAttribFormat(
            attr_idx, attr.num_components,
            attr.type, normalized, attr.offset
        );
      }
    }

    // Set the binding point to where the vertex buffer which holds the data is bound
    if constexpr(CreatePath == vertex_format_detail::Path_vertex_attrib_binding) {
      if(dsa_path) {
        glVertexArrayAttribBinding(vertex_array, attr_idx, attr.buffer_index);
      } else {
        glVertexAttribBinding(vertex_array, attr.buffer_index);
      }
    }
  };

  const auto format_vertex_array_object = [=](
      GLObject vertex_array, size_t attr_idx, const GLVertexFormatAttr& attr
  ) {
    auto off = attr.offsetAsPtr();

    // Setup the attribute's format
    if(attr.attr_type == GLVertexFormatAttr::Integer) {
      glVertexAttribIPointer(
          attr_idx, attr.num_components, attr.type, 0 /* TODO: pass proper stride here */,
          off /* TODO: attr.offset is relative, need to convert it to a cummulative one */
      );
    } else {
      // Check if the non-floating point data should be normalized
      //   when it's converted to floats
      GLboolean normalized = attr.normalized();

      glVertexAttribPointer(
          attr_idx, attr.num_components, attr.type, normalized,
          0, /* TODO: same as above - pass proper stride */
          off /* TODO: use cummulative offset instead of the stored (i.e. relative) one */
      );
    }
  };

  // ------------------------------------------------------------

  // Create the vertex array
  GLObject vertex_array = -1;
  if(dsa_path) {
    glCreateVertexArrays(1, &vertex_array);
  } else {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
  }

  for(size_t attr_idx = 0; attr_idx < GLVertexFormat::MaxVertexAttribs; attr_idx++) {
    const auto& attr = attribs.at(attr_idx);

    // Don't process empty (unused) attribute slots
    if(attr.attr_type == GLVertexFormatAttr::AttrInvalid) continue;

    // Found a used attribute slot!
    //   - Enable it in the VertexArray
    if(dsa_path) {
      glEnableVertexArrayAttrib(vertex_array, attr_idx);
    } else {
      glEnableVertexAttribArray(attr_idx);
    }

    // - Setup it's format
    //  -> Using a compile-time if statement we reduce code repetition AND slightly
    //     improve performance, whereas all other methods would sacrifice one or the
    //     other. Thanks C++17 :)
    //  -> The format_vertex_* functions are lambdas defined above the loop
    if constexpr(CreatePath == vertex_format_detail::Path_vertex_attrib_binding) {
      format_vertex_attrib_binding(vertex_array, attr_idx, attr);
    } else if(CreatePath == vertex_format_detail::Path_vertex_array_object) {
      format_vertex_array_object(vertex_array, attr_idx, attr);
    }

    // Make sure no error(s) have occured
    assert(glGetError() == GL_NO_ERROR);
  }

  // Unbind the VAO from the context.
  //   - They record global state, so subsequent
  //     seemingly unrelated function calls could
  //     leave it in an altered state
  if(!dsa_path) glBindVertexArray(0);

  return vertex_array;
}

auto GLVertexFormat::createVertexArray_vertex_attrib_binding() const -> GLVertexArray
{
  constexpr auto create_path = vertex_format_detail::Path_vertex_attrib_binding;
  auto arrayid = createVertexArrayGeneric_impl<create_path>(attributes_);

  GLVertexArray array;
  array.id_ = arrayid;
    
  return std::move(array);
}

auto GLVertexFormat::createVertexArray_vertex_array_object() const -> GLVertexArray
{
  constexpr auto create_path = vertex_format_detail::Path_vertex_array_object;
  auto arrayid = createVertexArrayGeneric_impl<create_path>(attributes_);

  GLVertexArray array;
  array.id_ = arrayid;
    
  return std::move(array);
}

void GLVertexFormat::invalidateCachedVertexSize()
{
  cached_vertex_size_ = std::nullopt;
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

auto GLVertexArray::bindVertexBuffer(
    unsigned index, const GLVertexBuffer& vertex_buffer,
    GLSize stride, GLSize offset
  ) -> GLVertexArray&
{
  assert(vertex_buffer.id() != GLNullObject &&
      "attempted to bind a null buffer to a vertex array!");
  assert(index < GLVertexFormat::MaxVertexBufferBindings &&
      "the index exceedes the maximum alowed number of vertex buffer bindings!");

  // GLSize is a signed type - so ensure the offset and size aren't negative
  assert(offset >= 0 && stride >= 0);

  // See note above method's declaration
  if(!ARB::vertex_attrib_binding()) throw VertexAttribBindingUnsupportedError();

  if(stride > GLVertexFormat::MaxVertexAttribStride) throw StrideExceedesMaxAllowedError();

  // Bind the buffer to the vertex array - utilizing
  //   DSA (direct state access) if available
  GLObject bufferid = vertex_buffer.id();
  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glVertexArrayVertexBuffer(id_, index, bufferid, offset, stride);
  } else {
    glBindVertexArray(id_);

    glBindVertexBuffer(index, bufferid, offset, stride);

    // Unbind the VAO for it's safety (since it saves global state
    //   at certain moments leaving it bound could cuase it's state
    //   to change unexpectedly)
    glBindVertexArray(0);
  }

  return *this;
}

auto GLVertexArray::bind() -> GLVertexArray&
{
  assert(id_ != GLNullObject && "attempted to bind() a null GLVertexArray!");

  glBindVertexArray(id_);

  return *this;
}

}
