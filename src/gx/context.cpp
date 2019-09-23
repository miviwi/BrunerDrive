#include <gx/context.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cstdio>

namespace brdrive {

GLContext::~GLContext()
{
}

auto GLContext::versionString() -> std::string
{
  return std::string((const char *)glGetString(GL_VERSION));
}

auto GLContext::version() -> GLVersion
{
  auto version_string = glGetString(GL_VERSION);

  GLVersion version;
  sscanf((const char *)version_string, "%d.%d", &version.major, &version.minor);

  return version;
}


}
