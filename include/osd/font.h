#pragma once

#include <types.h>

#include <vector>

namespace brdrive {

enum ExtendedCharacter : u8 {

};

struct FontGlyph {
};

class OSDBitmapFont {
public:
  OSDBitmapFont();

  auto loadBitmap1bppFile(const char *file) -> OSDBitmapFont&;
  auto loadBitmap1bpp(const void *data, size_t size) -> OSDBitmapFont&;

  auto pixelData() const -> const u8 *;
  auto pixelDataSize() const -> size_t;

private:
  // The font's pixels expanded to an 8bpp format
  //   (with values 0x00 and 0xFF) with the glyphs
  //   arranged in memory like so:
  //      <glyph 'A' row> ... <glyph 'A' row> <glyph 'B' row>
  std::vector<u8> font_pixels_;

  bool loaded_;
};

}
