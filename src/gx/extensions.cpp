#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <unordered_set>
#include <string_view>
#include <utility>

#include <cassert>

namespace brdrive {

thread_local int g_num_extensions = -1;
thread_local std::unordered_set<std::string_view> g_extensions;

auto queryExtension(const char *name) -> bool
{
  assert(gx_was_init() && "gx_init() must be called before this function can be used!");

  if(g_num_extensions < 0) {    // Need to initialize the 'g_extensions' set
    glGetIntegerv(GL_NUM_EXTENSIONS, &g_num_extensions);
    assert(g_num_extensions >= 0);

    for(int i = 0; i < g_num_extensions; i++) {
      std::string_view extension = (const char *)glGetStringi(GL_EXTENSIONS, i);
      g_extensions.emplace(extension);
    }
  }

  return g_extensions.find(name) != g_extensions.end();
}

// - 'ext' initially has a value < 0 to indicate it hasn't
//   been initialized yet
// - Otherwise, if 'ext' >= 0 it's value interpreted as a
//   boolean indicates the avilability of the extension
inline auto query_extension_cached(const char *name, int *ext) -> bool
{
  // HOT path - the avilability of the extension has already been cached
  if(*ext >= 0) return *ext;

  // Need to query 'g_extensions'
  return *ext = queryExtension(name);
}

namespace ARB {

thread_local int g_vertex_attrib_binding = -1;
auto vertex_attrib_binding() -> bool
{
  return query_extension_cached("GL_ARB_vertex_attrib_binding", &g_vertex_attrib_binding);
}

thread_local int g_separate_shader_objects = -1;
auto separate_shader_objects() -> bool
{
  return query_extension_cached("GL_ARB_separate_shader_objects", &g_separate_shader_objects);
}

thread_local int g_texture_storage = -1;
auto texture_storage() -> bool
{
  return query_extension_cached("GL_ARB_texture_storage", &g_texture_storage);
}

thread_local int g_direct_state_access = -1;
auto direct_state_access() -> bool
{
  return query_extension_cached("GL_ARB_direct_state_access", &g_direct_state_access);
}

}

namespace EXT {

thread_local int g_direct_state_access = -1;
auto direct_state_access() -> bool
{
  return query_extension_cached("GL_EXT_direct_state_access", &g_direct_state_access);
}

}

}
