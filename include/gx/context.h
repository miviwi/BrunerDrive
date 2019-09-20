#pragma once

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declaration
class IWindow;

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

  GLContext() = default;
  GLContext(const GLContext&) = delete;
  virtual ~GLContext();

  // Acquires and initializes the GLContext
  virtual auto acquire(IWindow *window) -> GLContext& = 0;
  // Makes this GLContext the 'current' context
  virtual auto makeCurrent() -> GLContext& = 0;

  // Swaps the front and back buffers
  virtual auto swapBuffers() -> GLContext& = 0;

  // Destroys the GLContext, which means
  //   acquire() can be called on it again
  virtual auto destroy() -> GLContext& = 0;

protected:
};

}
