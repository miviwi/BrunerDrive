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

  case GLType::i8:  return GL_BYTE;
  case GLType::i16: return GL_SHORT;

  case GLType::f16: return GL_HALF_FLOAT;
  case GLType::f32: return GL_FLOAT;

  default: ;       // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

GLVertexFormat::GLVertexFormat() :
  id_(GLNullObject),

  current_attrib_index_(0)
{
}

GLVertexFormat::GLVertexFormat(GLVertexFormat&& other) :
  GLVertexFormat()
{
  std::swap(id_, other.id_);
}

GLVertexFormat::~GLVertexFormat()
{
  if(id_ == GLNullObject) return;

  glDeleteVertexArrays(1, &id_);
}

auto GLVertexFormat::attr(
    int size, GLType type_, unsigned offset, AttrType attr_type
  ) -> GLVertexFormat&
{
  // Lazily allocate the backing OpenGL object
  if(id_ == GLNullObject) {
    glGenVertexArrays(1, &id_);
  }

  glBindVertexArray(id_);

  // Convert the GLType to an opengl GLenum value
  auto type = GLType_to_type(type_);
  if(type == GL_INVALID_ENUM) throw InvalidAttribTypeError();

  // Make sure attr_type is valid (i.e. one of Normalized, UnNormalized)
  assert((attr_type == AttrType::Normalized) || (attr_type == AttrType::UnNormalized) 
      && "invalid AttrType passed to attr()!");

  // Check if the (non-float) values should be normalized
  GLboolean normalized = (attr_type == AttrType::Normalized) ? GL_TRUE : GL_FALSE;

  glVertexAttribFormat(current_attrib_index_, size, type, normalized, offset);

  // Make sure the call succeeded
  assert(glGetError() == GL_NO_ERROR);

  // Save the attribute's properties to an internal data structure
  attr_formats_.at(current_attrib_index_) = GLAttrFormat {
    attr_type,

    current_attrib_index_,
    size, type, offset,
  };

  // Advance the current attribute index to the next slot
  current_attrib_index_++;

  return *this;
}

auto GLVertexFormat::iattr(int size, GLType type, unsigned offset) -> GLVertexFormat&
{
    // Lazily allocate the backing OpenGL object
  if(id_ == GLNullObject) {
    glGenVertexArrays(1, &id_);
  }

  glBindVertexArray(id_);

  // TODO: implement all of this :)
  /*
  // Convert the GLType to an opengl GLenum value
  auto type = GLType_to_type(type_);
  if(type == GL_INVALID_ENUM) throw InvalidAttribTypeError();

  // Check if the (non-float) values should be normalized
  GLboolean normalized = (attr_type == AttrType::Normalized) ? GL_TRUE : GL_FALSE;

  glVertexAttribFormat(current_attrib_index_, size, type, normalized, offset);

  // Make sure the call succeeded
  assert(glGetError() == GL_NO_ERROR);
  */

  // Advance the current attribute index to the next slot
  current_attrib_index_++;

  return *this;
}

}
