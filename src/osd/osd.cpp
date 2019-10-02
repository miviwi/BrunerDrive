#include <osd/osd.h>

namespace brdrive {

static bool g_osd_was_init = false;

void osd_init()
{
  g_osd_was_init = true;
}

void osd_finalize()
{
  g_osd_was_init = false;
}

auto osd_was_init() -> bool
{
  return g_osd_was_init;
}

}
