#include <gx/context.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>
#include <cstdio>

namespace brdrive {

GLContext::~GLContext()
{
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


}
