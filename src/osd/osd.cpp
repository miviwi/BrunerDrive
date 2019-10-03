#include <osd/osd.h>
#include <osd/surface.h>
#include <gx/program.h>

namespace brdrive {

static bool g_osd_was_init = false;

void osd_init()
{
  OSDSurface::s_surface_program = new GLProgram();

  g_osd_was_init = true;
}

void osd_finalize()
{
  delete OSDSurface::s_surface_program;
  OSDSurface::s_surface_program = nullptr;

  g_osd_was_init = false;
}

auto osd_was_init() -> bool
{
  return g_osd_was_init;
}

}
