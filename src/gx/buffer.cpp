#include <gx/buffer.h>
#include <gx/extensions.h>
#include <gx/texture.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

namespace brdrive {

inline constexpr auto GLBufferUsage_to_usage(GLBuffer::Usage usage) -> GLEnum
{
  switch(usage) {
  case GLBuffer::StaticDraw:  return GL_STATIC_DRAW;
  case GLBuffer::DynamicDraw: return GL_DYNAMIC_DRAW;
  case GLBuffer::StreamDraw:  return GL_STREAM_DRAW;

  case GLBuffer::StaticCopy:  return GL_STATIC_COPY;
  case GLBuffer::DynamicCopy: return GL_DYNAMIC_COPY;
  case GLBuffer::StreamCopy:  return GL_STREAM_COPY;

  case GLBuffer::StaticRead:  return GL_STATIC_READ;
  case GLBuffer::DynamicRead: return GL_DYNAMIC_READ;
  case GLBuffer::StreamRead:  return GL_STREAM_READ;
  }

  return GL_INVALID_ENUM;
}

inline constexpr auto GLBufferUsage_is_static(GLBuffer::Usage usage) -> bool
{
  auto frequency = (usage & GLBuffer::FrequencyMask) >> GLBuffer::FrequencyShift;
  return frequency == GLBuffer::Static;
}

GLBuffer::GLBuffer(GLEnum bind_target) :
  id_(GLNullObject),
  bind_target_(bind_target),
  size_(~0), usage_(UsageInvalid)
{
}

GLBuffer::~GLBuffer()
{
  if(id_ == GLNullObject) return;

  glDeleteBuffers(1, &id_);
}

auto GLBuffer::alloc(GLSize size, Usage usage, const void *data) -> GLBuffer&
{
  GLbitfield storage_flags = GL_DYNAMIC_STORAGE_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT;
  if(GLBufferUsage_is_static(usage)) {
    if(!data) throw NoDataForStaticBufferError();

    // Make the buffer's data immutable by the CPU
    storage_flags &= ~GL_DYNAMIC_STORAGE_BIT;
  }

  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glCreateBuffers(1, &id_);

    if(ARB::buffer_storage()) {
      glNamedBufferStorage(id_, size, data, storage_flags);
    } else {
      glNamedBufferData(id_, size, data, GLBufferUsage_to_usage(usage));
    }
  } else {
    glGenBuffers(1, &id_);
    bindSelf();   // glBindBuffer(...)

    if(ARB::buffer_storage()) {
      glBufferStorage(bind_target_, size, data, storage_flags);
    } else {
      glBufferData(bind_target_, size, data, GLBufferUsage_to_usage(usage));
    }
  }

  assert(glGetError() == GL_NO_ERROR);

  // Initialize internal variables
  size_ = size; usage_ = usage;

  return *this;
}

auto GLBuffer::upload(const void *data) -> GLBuffer&
{
  assert(id_ != GLNullObject && "attempted to upload() to a null buffer!");

  // Make sure the buffer was created with proper usage
  if(GLBufferUsage_is_static(usage_)) throw UploadToStaticBufferError();

  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glNamedBufferSubData(id_, 0, size_, data);
  } else {
    bindSelf();   // glBindBuffer(...)
    glBufferSubData(bind_target_, 0, size_, data);
  }

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLBuffer::id() const -> GLObject
{
  return id_;
}

auto GLBuffer::bindTarget() const -> GLEnum
{
  return bind_target_;
}

void GLBuffer::bindSelf()
{
  assert(id_ != GLNullObject && "attempted to use a null buffer!");

  glBindBuffer(bind_target_, id_);
}

GLVertexBuffer::GLVertexBuffer() :
  GLBuffer(GL_ARRAY_BUFFER)
{
}

GLIndexBuffer::GLIndexBuffer() :
  GLBuffer(GL_ELEMENT_ARRAY_BUFFER)
{
}

GLUniformBuffer::GLUniformBuffer() :
  GLBuffer(GL_UNIFORM_BUFFER)
{
}

inline constexpr auto XferDirection_to_bind_target(
    GLPixelBuffer::XferDirection xfer_direction
  ) -> GLEnum
{
  switch(xfer_direction) {
  case GLPixelBuffer::Upload:   return GL_PIXEL_UNPACK_BUFFER;
  case GLPixelBuffer::Download: return GL_PIXEL_PACK_BUFFER;

  default: assert(0);     // Unreachable
  }

  return GL_INVALID_ENUM;
}

// TODO: merge these functions with the ones in gx/texture.cpp (?)
inline constexpr auto GLFormat_to_format(GLFormat format) -> GLenum
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

// TODO: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
inline constexpr auto GLType_to_type(GLType type) -> GLenum
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

GLPixelBuffer::GLPixelBuffer(XferDirection xfer_direction) :
  GLBuffer(XferDirection_to_bind_target(xfer_direction)),
  xfer_direction_(xfer_direction)
{
}

auto GLPixelBuffer::uploadTexture(
    GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset_
  ) -> GLPixelBuffer&
{
  assert(id_ != GLNullObject && "attempted to uploadTexture() from a null GLPixelBuffer!");
  
  // Make sure this buffer is a GLPixelBuffer(Upload)
  static constexpr XferDirection direction = Upload;
  if(xfer_direction_ != direction) throw InvalidXferDirectionError<direction>();

  auto gl_format = GLFormat_to_format(format);
  auto gl_type   = GLType_to_type(type);

  if(gl_format == GL_INVALID_ENUM || gl_type == GL_INVALID_ENUM)
    throw GLTexture::InvalidFormatTypeError();

  // Convert the offset to a void* as glTextureImage* functions
  //   are overloaded as such when uploading from a buffer
  auto offset = (void *)offset_;

  // Have to bind the buffer to the context
  //   whether DSA is available or not
  bindSelf();

  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    switch(tex.dimensions()) {
    case GLTexture::TexImage2D:
      glTextureSubImage2D(
          tex.id(), level, 0, 0, tex.width(), tex.height(),
          gl_format, gl_type, offset /* The data source is a GLPixelBuffer(Upload) */
      );
      break;

    default: assert(0 && "TODO: unimplemented!"); break;
    }
  } else {
    glBindTexture(tex.bindTarget(), tex.id());

    switch(tex.dimensions()) {
    case GLTexture::TexImage2D:
      glTextureSubImage2D(
          tex.bindTarget(), level, 0, 0, tex.width(), tex.height(),
          gl_format, gl_type, offset /* The data source is a GLPixelBuffer(Upload) */
      );
      break;

    default: assert(0 && "TODO: unimplemented!"); break;
    }
  }

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLPixelBuffer::downloadTexture(
    const GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset_
  ) -> GLPixelBuffer&
{
  assert(id_ != GLNullObject && "attempted to downloadTexture() to a null GLPixelBuffer!");

  // Make sure this buffer is a GLPixelBuffer(Download)
  static constexpr XferDirection direction = Download;
  if(xfer_direction_ != direction) throw InvalidXferDirectionError<direction>();

  auto gl_format = GLFormat_to_format(format);
  auto gl_type   = GLType_to_type(type);

  if(gl_format == GL_INVALID_ENUM || gl_type == GL_INVALID_ENUM)
    throw GLTexture::InvalidFormatTypeError();

  // Convert the offset to a void* as glGetTextureImage() function
  //   is overloaded as such when downloading to a buffer
  auto offset = (void *)offset_;

  // Have to bind the buffer to the context
  //   whether DSA is available or not
  bindSelf();

  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glGetTextureImage(
        tex.id(), level, gl_format, gl_type, size_,
        offset /* The data destination buffer is a GLPixelBuffer(Download) */
    );
  } else {
    auto bind_target = tex.bindTarget();

    // Save the currently bound texture to restore it after the
    //  glGetTexImage() call is made
    //   TODO: somehow restore the current image unit's binding
    //         without querying OpenGL's state...

    auto pname = ([bind_target]() -> GLEnum {
      switch(bind_target) {
      case GL_TEXTURE_1D: return GL_TEXTURE_BINDING_1D;
      case GL_TEXTURE_1D_ARRAY: return GL_TEXTURE_BINDING_1D_ARRAY;

      case GL_TEXTURE_2D: return GL_TEXTURE_BINDING_2D;
      case GL_TEXTURE_2D_ARRAY: return GL_TEXTURE_BINDING_2D_ARRAY;

      case GL_TEXTURE_CUBE_MAP: return GL_TEXTURE_BINDING_CUBE_MAP;

      case GL_TEXTURE_3D: return GL_TEXTURE_BINDING_3D;

      default: ;    // Fallthrough (silence warnings)
      }

      return GL_INVALID_ENUM;
    })();

    int current_tex = -1;

    glGetIntegerv(pname, &current_tex);
    assert(glGetError() == GL_NO_ERROR);
    
    // Bind the source texture for the download...
    glBindTexture(bind_target, tex.id());

    // ...actually perform the download...
    //  - Use the safer (in this context) version of glGetTexImage()
    glGetnTexImage(
        bind_target, level, gl_format, gl_type, size_,
        offset /* The data destination buffer is a GLPixelBuffer(Download) */
    );

    assert(glGetError() == GL_NO_ERROR);

    // ...and restore the previously bound texture
    glBindTexture(bind_target, current_tex);
  }

  return *this;
}

}
