#include <gx/gx.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

namespace brdrive {

bool g_gx_was_init = false;
GLObject g_null_vao = GLNullObject;

void gx_init()
{
  auto result = gl3wInit();
  if(result != GL3W_OK) {
    printf("gl3wInit(): %d\n", result);

    throw GL3WInitError();
  }

  glCreateVertexArrays(1, &g_null_vao);
  glBindVertexArray(g_null_vao);

  g_gx_was_init = true;
}

void gx_finalize()
{
  glBindVertexArray(0);
  glDeleteVertexArrays(1, &g_null_vao);

  g_gx_was_init = false;
}

auto gx_was_init() -> bool
{
  return g_gx_was_init;
}

}
