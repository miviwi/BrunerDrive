#pragma once

#include <gx/gx.h>

namespace brdrive {

auto queryExtension(const char *name) -> bool;

namespace ARB {
auto vertex_attrib_binding() -> bool;
auto separate_shader_objects() -> bool;
auto texture_storage() -> bool;
auto buffer_storage() -> bool;
auto direct_state_access() -> bool;
}

namespace EXT {
auto direct_state_access() -> bool;
}

}
