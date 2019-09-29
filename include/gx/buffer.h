#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declarations
class GLBufferMapping;
class GLVertexArray;
class GLTexture;

class GLBuffer {
public:
  // The values are created such that
  //     Usage: 0000 ffaa
  //  where each digit/symbol corresponds
  //  to a single bit and
  //    - 'f' is the frequency of access
  //      { Static, Dynamic, Stream }
  //    - 'a' is the type of access
  //      { Read, Copy, Draw }
  enum Usage {
    Static = 0, Dynamic = 1, Stream = 2,
    Read = 0, Copy = 1, Draw = 2,

    FrequencyMask  = 0b00001100,
    AccessTypeMask = 0b00000011,

    FrequencyShift  = 2,
    AccessTypeShift = 0,

    StaticRead  = (Static<<FrequencyShift)|Read,
    DynamicRead = (Dynamic<<FrequencyShift)|Read,
    StreamRead  = (Stream<<FrequencyShift)|Read,

    StaticCopy  = (Static<<FrequencyShift)|Copy,
    DynamicCopy = (Dynamic<<FrequencyShift)|Copy,
    StreamCopy  = (Stream<<FrequencyShift)|Copy,

    StaticDraw  = (Static<<FrequencyShift)|Draw,
    DynamicDraw = (Dynamic<<FrequencyShift)|Draw,
    StreamDraw  = (Stream<<FrequencyShift)|Draw,

    UsageInvalid = ~0,
  };

  static constexpr unsigned MaxBindIndex = 16;

  enum MapFlags : u32 {
    MapRead  = (1<<0),
    MapWrite = (1<<1),

    MapInvalidateRange  = (1<<2),
    MapInvalidateBuffer = (1<<3),
    MapFlushExplicit    = (1<<4),
    MapUnsynchronized   = (1<<5),
    MapPersistent       = (1<<6),
    MapCoherent         = (1<<7),
  };

  struct NoDataForStaticBufferError : public std::runtime_error {
    NoDataForStaticBufferError() :
      std::runtime_error("a 'Static' GLBuffer MUST be supplied with data upon allocation!")
    { }
  };

  struct UploadToStaticBufferError : public std::runtime_error {
    UploadToStaticBufferError() :
      std::runtime_error("cannot upload() to a buffer with 'Static' usage frequency!")
    { }
  };

  struct InvalidBindingIndexError : public std::runtime_error {
    InvalidBindingIndexError() :
      std::runtime_error("the 'index' for an indexed bind must be in the range [0;MaxBindIndex]")
    { }
  };
  
  struct OffsetExceedesSizeError : public std::runtime_error {
    OffsetExceedesSizeError() :
      std::runtime_error("the offset specified exceedes the buffer's size!")
    { }
  };

  struct OffsetAlignmentError : public std::runtime_error {
    OffsetAlignmentError() :
      std::runtime_error("the offset MUST be aligned on a target specific boundary!"
          " (the alignment can be queried via the buffer's bindOffsetAlignment() method)")
    { }
  };

  struct SizeExceedesBuffersSizeError : public std::runtime_error {
    SizeExceedesBuffersSizeError() :
      std::runtime_error("the requested size is > the buffer's size (possibly reduced by the passed 'offset'")
    { }
  };

  struct InvalidMapFlagsError : public std::runtime_error {
    InvalidMapFlagsError() :
      std::runtime_error("the flags MUST contain at least one of { MapRead, MapWrite }")
    { }
  };

  struct MapFailedError : public std::runtime_error {
    MapFailedError() :
      std::runtime_error("the call to glMapBuffer() failed")
    { }
  };

  GLBuffer(const GLBuffer&) = delete;
  GLBuffer(GLBuffer&&) = delete;
  virtual ~GLBuffer();

  auto alloc(GLSize size, Usage usage, const void *data = nullptr) -> GLBuffer&;
  auto upload(const void *data) -> GLBuffer&;

  auto map(u32 /* MapFlags */ flags, intptr_t offset = 0, GLSizePtr size = 0) -> GLBufferMapping;

  // Also called in ~GLBufferMapping, so a manual
  //   call ism't needed
  auto unmap(GLBufferMapping& mapping) -> GLBuffer&;

  auto id() const -> GLObject;
  auto bindTarget() const -> GLEnum;
  auto size() const -> GLSize;

protected:
  GLBuffer(GLEnum bind_target);

  // Binds this buffer to the context
  void bindSelf();
  // Binds 0 to bind_tagret_ (which,
  //   effectively, unbinds any buffer
  //   from said target)
  void unbindSelf(); 

  GLObject id_;
  GLEnum bind_target_;

  GLSize size_;
  Usage usage_;

  // Set to 'true' if map() was called
  //   and it's GLBufferMapping is still
  //   in scope
  bool mapped_;
};

class GLBufferMapping {
public:
  struct MappingNotFlushableError : public std::runtime_error {
    MappingNotFlushableError() :
      std::runtime_error("flush() can be used only when the buffer"
          " was mapped with the GLBuffer::MapFlushExplicit flag!")
    { }
  };

  struct FlushRangeError : public std::runtime_error {
    FlushRangeError() :
      std::runtime_error("attempted to flush the buffer past the mapped range!"
          " (either the offset > mapped_size | size > mapped_size | offset+size > mapped_size)")
    { }
  };

  GLBufferMapping(const GLBufferMapping&) = delete;
  ~GLBufferMapping();

  // Returns a pointer to the start of
  //   the mapped buffer's range
  auto get() -> void *;
  auto get() const -> const void *;

  // Helpers to eliminate need for
  //   casting void* from the
  //   non-templated get()
  template <typename T>
  auto get() -> T *
  {
    return (T*)get();
  }
  template <typename T>
  auto get() const -> const T *
  {
    return (const T *)get();
  }

  // Returns the n-th element of the
  //   mapped buffer's range as if
  //   it is an array of T
  template <typename T>
  auto at(size_t n) -> T&
  {
    return *(get<T>() + n);
  }
  template <typename T>
  auto at(size_t n) const -> const T&
  {
    return *(get<T>() + n);
  }

  // Returns 'true' if the mapping hasn't been unmapped
  operator bool() { return ptr_; }

  // Ensures data written by the host in the range [offset;offset+size]
  //   becomes visible on the device
  auto flush(intptr_t offset = 0, GLSizePtr length = 0) -> GLBufferMapping&;

  void unmap();

private:
  friend GLBuffer;

  GLBufferMapping(GLBuffer& buffer, u32 /* MapFlags */ flags, void *ptr);

  GLBuffer& buffer_;
  u32 /* GLBuffer::MapFlags */ flags_;
  void *ptr_;
};

class GLVertexBuffer : public GLBuffer {
public:
  GLVertexBuffer();
};

// NOTE: an index buffer can be allocated (strictly speaking
//       only without DSA) ONLY while a vertex array is bound
class GLIndexBuffer : public GLBuffer {
public:
  GLIndexBuffer();
};

class GLUniformBuffer : public GLBuffer {
public:
  GLUniformBuffer();

  // When 'size' isn't specified the entire buffer (starting at 'offset') will be bound by the call
  auto bindToIndex(unsigned index, intptr_t offset = 0, GLSizePtr size = 0) -> GLUniformBuffer&;
};

class GLPixelBuffer : public GLBuffer {
public:
  enum XferDirection {
    Upload, Download,
  };

  template <XferDirection xfer_direction>
  struct InvalidXferDirectionError : public std::runtime_error {
    InvalidXferDirectionError() :
      std::runtime_error(error_message())
    { }

  private:
    static auto error_message() -> const char *
    {
      if constexpr(xfer_direction == Upload) {
        return "used a GLPixelBuffer(Download) for an upload() operation";
      } else if(xfer_direction == Download) {
        return "used a GLPixelBuffer(Upload) for a download() operation";
      }

      return nullptr;   // Unreachable
    }
  };
  
  GLPixelBuffer(XferDirection xfer_direction);

  // Upload the buffer's data to the texture
  //   - 'format' and 'type' describe the format of the BUFFER's pixels
  //   - 'offset' is a byte-offset into the buffer where the data
  //      to be uploaded resides
  //   - throws GLTexture::InvalidFormatTypeError() when the specified
  //     type or format/type combination is invalid
  auto uploadTexture(
      GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset = 0
    ) -> GLPixelBuffer&;
  // Fill the buffer with the texture's data
  //   - 'format' and 'type' describe the format of the pixels in the
  //      BUFFER after the download completes
  //   - 'offset' is a byte-offset into the buffer where the data
  //      will be written
  //   - throws GLTexture::InvalidFormatTypeError() when the specified
  //     type or format/type combination is invalid
  auto downloadTexture(
      const GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset = 0
    ) -> GLPixelBuffer&;

private:
  XferDirection xfer_direction_;
};

}
