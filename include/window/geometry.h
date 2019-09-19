#pragma once

#include <types.h>
#include <util/primitive.h>

namespace brdrive {

struct Geometry {
  u16 x, y;
  u16 w, h;

  static auto xy(u16 x, u16 y) -> Geometry
  {
    return { x, y, 0xFFFF, 0xFFFF };
  }
};

}
