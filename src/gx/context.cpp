#include <gx/context.h>
#include <gx/texture.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>

#include <new>

namespace brdrive {

static void MessageCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam
  )
{
  auto prefix = type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "";

  fprintf(stderr,
    "OpenGL: %s type = 0x%x, severity = 0x%x, message = %s\n",
    prefix, type, severity, message
  );
}

GLContext::GLContext() :
  was_acquired_(false),
  tex_image_units_(nullptr)
{
  // Allocate backing memory via malloc() because GLTexImageUnit's constructor requires
  //   an argument and new[] doesn't support passing per-instance args
  tex_image_units_ = (GLTexImageUnit *)malloc(GLNumTexImageUnits * sizeof(GLTexImageUnit));
  for(unsigned i = 0; i < GLNumTexImageUnits; i++) {
    auto unit = tex_image_units_ + i;

    // Use placement-new to finalize creation of the object
    new(unit) GLTexImageUnit(i);
  }
}

GLContext::~GLContext()
{
  // Because malloc() was used to allocate 'tex_image_units_' we need to call
  //   the destructors on each of the GLTexImageUnits manually...
  for(unsigned i = 0; i < GLNumTexImageUnits; i++) {
    auto unit = tex_image_units_ + i;
    unit->~GLTexImageUnit();
  }

  // ...and release the memory with free()
  free(tex_image_units_);
}

auto GLContext::dbg_EnableMessages() -> GLContext&
{
  int context_flags = -1;
  glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);

  if(!(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) throw NotADebugContextError();

  // Enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  glDebugMessageCallback(MessageCallback, nullptr);

  return *this;
}

auto GLContext::versionString() -> std::string
{
  assert(gx_was_init() && "gx_init() must be called before using this method!");

  return std::string((const char *)glGetString(GL_VERSION));
}

auto GLContext::version() -> GLVersion
{
  assert(gx_was_init() && "gx_init() must be called before using this method!");

  auto version_string = glGetString(GL_VERSION);

  GLVersion version;
  sscanf((const char *)version_string, "%d.%d", &version.major, &version.minor);

  return version;
}

auto GLContext::texImageUnit(unsigned slot) -> GLTexImageUnit&
{
  assert(was_acquired_ && "the context must've been acquire()'d to use it's texImageUnits!");
  assert(slot < GLNumTexImageUnits && "'slot' must be < GLNumTexImageUnits!");

  return tex_image_units_[slot];
}

}
