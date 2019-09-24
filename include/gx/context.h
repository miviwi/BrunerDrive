#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declarations
class IWindow;

class GLTexImageUnit;
class GLTexture2D;
class GLSampler;
// --------------------

// Handle to the underlying OS-specific OpenGL context structure
using GLContextHandle = void *;

struct GLVersion {
  int major, minor;
};

class GLContext {
public:
  struct NoSuitableFramebufferConfigError : public std::runtime_error {
    NoSuitableFramebufferConfigError() :
      std::runtime_error("no siutable frmebuffer config could be found!")
    { }
  };

  struct AcquireError : public std::runtime_error {
    AcquireError() :
      std::runtime_error("failed to acquire the GLContext!")
    { }
  };

  struct MakeCurrentError : public std::runtime_error {
    MakeCurrentError() :
      std::runtime_error("failed to make the GLContext the current context!")
    { }
  };

  struct NotADebugContextError : public std::runtime_error {
    NotADebugContextError() :
      std::runtime_error("the operation can only be performed on a debug OpenGL context!")
    { }
  };

  GLContext();
  GLContext(const GLContext&) = delete;
  virtual ~GLContext();

  // Acquires and initializes the GLContext
  virtual auto acquire(
      IWindow *window, GLContext *share = nullptr
    ) -> GLContext& = 0;

  // Makes this GLContext the 'current' context
  virtual auto makeCurrent() -> GLContext& = 0;

  // Swaps the front and back buffers
  virtual auto swapBuffers() -> GLContext& = 0;

  // Destroys the GLContext, which means
  //   acquire() can be called on it again
  virtual auto destroy() -> GLContext& = 0;

  // Returns a handle to the underlying OS-specific
  //   OpenGL context or nullptr if acquire() hasn't
  //   yet been called
  virtual auto handle() -> GLContextHandle = 0;

  auto texImageUnit(unsigned slot) -> GLTexImageUnit&;

  // Can only be called AFTER gx_init()!
  auto dbg_EnableMessages() -> GLContext&;

  auto versionString() -> std::string;
  auto version() -> GLVersion;

protected:
  bool was_acquired_;

private:
  GLTexImageUnit *tex_image_units_;  // Array
};

}
