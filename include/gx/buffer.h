#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declarations
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

  GLBuffer(const GLBuffer&) = delete;
  virtual ~GLBuffer();

  auto alloc(GLSize size, Usage usage, const void *data = nullptr) -> GLBuffer&;
  auto upload(const void *data) -> GLBuffer&;

  auto id() const -> GLObject;
  auto bindTarget() const -> GLEnum;

protected:
  GLBuffer(GLEnum bind_target);

  // Binds this buffer to the context
  void bindSelf();

  GLObject id_;
  GLEnum bind_target_;

  GLSize size_;
  Usage usage_;
};

class GLVertexBuffer : public GLBuffer {
public:
  GLVertexBuffer();
};

class GLIndexBuffer : public GLBuffer {
public:
  GLIndexBuffer();
};

class GLUniformBuffer : public GLBuffer {
public:
  GLUniformBuffer();
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
