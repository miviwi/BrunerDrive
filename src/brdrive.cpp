#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <brdrive.h>
#include <types.h>
#include <window/geometry.h>
#include <window/color.h>
#include <window/window.h>
#include <gx/gx.h>
#include <gx/extensions.h>
#include <gx/context.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/texture.h>
#include <gx/program.h>
#include <gx/fence.h>
#include <x11/x11.h>
#include <x11/connection.h>
#include <x11/window.h>
#include <x11/event.h>
#include <x11/glx.h>
#include <osd/osd.h>
#include <osd/font.h>
#include <osd/surface.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <GL/gl3w.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>

auto load_font(const std::string& file_name) -> std::optional<std::vector<uint8_t>>
{
  auto fd = open(file_name.data(), O_RDONLY);
  if(fd < 0) return std::nullopt;

  struct stat st;
  if(fstat(fd, &st) < 0) return std::nullopt;

  std::vector<uint8_t> font(st.st_size);
  if(read(fd, font.data(), font.size()) < 0) {
    close(fd);
    return std::nullopt;
  }

  close(fd);
  return std::move(font);
}

int main(int argc, char *argv[])
{
  using namespace brdrive;
  x11_init();

  X11Window window;
  X11EventLoop event_loop;

  window
    .geometry({ 0, 0, 960, 960 })
    .background(Color(1.0f, 0.0f, 1.0f, 0.0f))
    .create()
    .show();

  event_loop
    .init(&window);

  GLXContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent();

  gx_init();
  osd_init();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  gl_context
    .dbg_EnableMessages();

  printf("OpenGL %s\n\n", gl_context.versionString().data());

  GLShader vert(GLShader::Vertex);
  GLShader frag(GLShader::Fragment);

  vert
    .source(R"VERT(
layout(location = 0) in vec3 viColor;

out Vertex {
  vec3 Position;
  vec3 ScreenPosition;
  vec3 Color;
  vec2 UV;
  float Character;
} vo;

const float FontSize = 0.25f;

// The positions of a single glyph's vertices
//   at the screen's top-left corner
const vec4 PositionsOffsetVector = vec4(vec2(0.5f*FontSize, 1.0f-FontSize), 0.0f, 0.0f);
const vec4 Positions[4] = vec4[](
  vec4(-1.0f, 1.0f, 0.0f, 1.0f),
  vec4(-1.0f, 0.0f, 0.0f, 1.0f) + PositionsOffsetVector.wyww,
  vec4(-1.0f, 0.0f, 0.0f, 1.0f) + PositionsOffsetVector.xyww,
  vec4(-1.0f, 1.0f, 0.0f, 1.0f) + PositionsOffsetVector.xwww
);

// Positions of a full-screen quad's vertices
const vec4 ScreenPositions[4] = vec4[](
  vec4(-1.0f, +1.0f, 0.1f, 1.0f),
  vec4(-1.0f, -1.0f, 0.1f, 1.0f),
  vec4(+1.0f, -1.0f, 0.1f, 1.0f),
  vec4(+1.0f, +1.0f, 0.1f, 1.0f)
);

// UV coordinates which encompass
//   a single character in 'usFontTopaz'
const vec2 UVs[4] = vec2[](
  vec2(0.0f, 0.0f/256.0f),
  vec2(0.0f, 1.0f/256.0f),
  vec2(1.0f, 1.0f/256.0f),
  vec2(1.0f, 0.0f/256.0f)
);

uniform isamplerBuffer usStrings;
uniform isamplerBuffer usStringXYPositionsOffsetsLengths;

// Gives an integer which is the index of the glyph
//   currently being rendered
int OffsetInString() { return gl_VertexID >> 2; }
// Gives an integer in the range [0;3] which is an
//   index of the current glyph's quad vertex
//   starting from the top-left and advancing
//   counter-clockwise
int GlyphQuad_VertexID() { return gl_VertexID & 3; }

const float ScreenAspect = 16.0f/9.0f;
const vec2 InvResolution = vec2(1.0f/512.0f, -1.0f/512.0f);

const float TexCharHeight = 255.0f/256.0f;

const vec2 ScreenCharDimensions = vec2(0.125f, 0.25f);

void main()
{
  // Fetch the string's properties from a texture, that is:
  //   * position (expressed in pixels with 0,0 at the top left corner)
  //   * the offset in the usStrings texture at which the sitring's
  //     characters can be found
  //   * the string's length
  //  and unpack them for convenient access
  ivec4 string_xy_off_len = texelFetch(usStringXYPositionsOffsetsLengths, gl_InstanceID);
  vec2 string_xy = vec2(string_xy_off_len.rg) * InvResolution;
  int string_offset = string_xy_off_len.b;
  int string_length = string_xy_off_len.a;

  int string_character_num = OffsetInString();
  int vert_id = GlyphQuad_VertexID();

  // Because of instancing, more characters can be rendered
  //   than there are in a given string, in the above case
  //   cull the additional glyphs
  if(string_character_num >= string_length) {
    gl_Position = vec4(0.0f, 0.0f, 0.0f, -1.0f);
    return;
  }

  // The index of the string's character being rendered
  int character_num = string_offset + string_character_num;

  int char = texelFetch(usStrings, character_num).r;
  float char_t_offset = float(char) * TexCharHeight;

  // Compute the offset of the glyph being rendered relative to the start of the string
  vec2 glyph_advance = vec2(ivec2(string_character_num, -gl_InstanceID)) * ScreenCharDimensions;

  // Compute the needed output data...
  vec4 pos = Positions[vert_id];
  vec2 uv = UVs[vert_id] - vec2(0.0f, char_t_offset);
  vec3 projected_pos = vec3(pos.x * ScreenAspect, pos.yz);
  vec4 screen_pos = ScreenPositions[vert_id];

  // ...and assign it
  vo.Position = projected_pos;
  vo.ScreenPosition = screen_pos.xyz;
  vo.Color = viColor;
  vo.UV = uv;
  vo.Character = char;

  // Position the vertex according to the given offset
  //   and posiiton in the string being rendered
  gl_Position = pos + vec4(string_xy + glyph_advance, 0.0f, 0.0f);
}
)VERT")
  ;

  frag
    .source(R"FRAG(
in Vertex {
  vec3 Position;
  vec3 ScreenPosition;
  vec3 Color;
  vec2 UV;
  float Character;
} fi;

#if defined(NO_BLEND)
#  define OUTPUT_CHANNELS vec3
#else
#  define OUTPUT_CHANNELS vec4
#endif
out OUTPUT_CHANNELS foFragColor;

uniform sampler2D usFontTopaz;
uniform vec3 uvFontColor;

void main()
{
  const float Radius = 0.5f;

  const vec3 Red    = vec3(1.0f, 0.0f, 0.0f);
  const vec3 Blue   = vec3(0.08f, 0.4f, 0.75f);
  const vec3 LBlue  = vec3(0.4f, 0.7f, 0.96f);
  const vec3 Yellow = vec3(1.0f, 1.0f, 0.0f);
  const vec3 Lime   = vec3(0.75f, 1.0f, 0.0f);
  const vec3 Orange = vec3(1.0f, 0.4f, 0.0f);
  const vec3 Black  = vec3(0.0f, 0.0f, 0.0f);
  const vec3 White  = vec3(1.0f, 1.0f, 1.0f);

  float glyph_sample = texture(usFontTopaz, fi.UV).r;
  float alpha = glyph_sample;
  float alpha_mask = 1.0f-glyph_sample;

  vec3 glyph_color = uvFontColor * glyph_sample;

#if defined(NO_BLEND)
  // Do the equivalent of an old-school alpha test (NOT - blend!)
  if(alpha_mask < 0.0f) discard;

  foFragColor = glyph_color;
#else
  foFragColor = vec4(glyph_color, alpha);
#endif
}
)FRAG");

  try {
    vert.compile();
    frag.compile();
  } catch(const std::exception& e) {
    auto vert_info_log = vert.infoLog();
    auto frag_info_log = frag.infoLog();

    if(vert_info_log) {
      puts(vert_info_log->data());
    }

    if(frag_info_log) {
      puts(frag_info_log->data());
    }

    return -2;
  }

  GLProgram gl_program;

  gl_program
    .attach(vert)
    .attach(frag);

  try {
    gl_program.link();
  } catch(const std::exception& e) {
    puts(gl_program.infoLog()->data());
    return -2;
  }

  gl_program
    .detach(vert)
    .detach(frag);

  GLProgram compute_shader_program;

  GLShader compute_shader_program_shader(GLShader::Compute);
  compute_shader_program_shader
    .glslVersion(430)
    .source(R"COMPUTE(
uniform writeonly image2D uiComputeOut;

uniform float ufWavePeriod;

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

void main()
{
  float work_group_x = float(gl_WorkGroupID.x) / 4096.0f;     // normalize to [0;1]

  float wave_sin = sin(work_group_x * ufWavePeriod*(1.0f/2.0f));
  float wave_cos = cos(work_group_x * ufWavePeriod);

  float blue = (wave_sin < 0.0f) && (wave_cos < 0.0f) ? 1.0f : 0.0f;

  vec2 wave = pow(vec2(wave_sin, wave_cos), vec2(2.0f));

  imageStore(uiComputeOut, ivec2(int(gl_WorkGroupID.x), 0), vec4(wave, blue, 1));
}
)COMPUTE");

  try {
    compute_shader_program_shader.compile();
  } catch(const std::exception& e) {
    auto info_log = compute_shader_program_shader.infoLog();

    if(info_log) {
      puts(info_log->data());
    }

    return -2;
  }

   compute_shader_program
     .attach(compute_shader_program_shader);

   try {
    compute_shader_program.link();
  } catch(const std::exception& e) {
    if(!compute_shader_program.infoLog()) return -2;

    puts(compute_shader_program.infoLog()->data());
    return -2;
  }

  GLTexture2D compute_output_tex;
  compute_output_tex
    .alloc(4096, 1, 1, rgba8);

  glBindImageTexture(0, compute_output_tex.id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

  compute_shader_program
    .uniform("uiComputeOut", gl_context.texImageUnit(0))
    .uniform("ufWavePeriod", 1024.0f);

  std::chrono::high_resolution_clock clock;
  auto start = clock.now();

  compute_shader_program.use();
  glDispatchCompute(4096, 1, 1);

  auto end = clock.now();

  auto ms_counts = std::chrono::milliseconds(1).count(); 

  printf("\ncompute_shader_program took: %dus\n\n",
      std::chrono::duration_cast<std::chrono::microseconds>(end-start));

  GLFence compute_fence;
  compute_fence
    .fence()
    .block();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  glClearColor(1.0f, 1.0f, 0.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT);

  const char topaz_test_string[] = "hello, world!ASDF1234567890";
  GLBufferTexture string_buf_tex;
  string_buf_tex
    .alloc(
        sizeof(topaz_test_string), GLBuffer::DynamicRead, GLBuffer::MapWrite|GLBuffer::ClientStorage
    );
  {
    auto string_buf_tex_mapping = string_buf_tex.map(
        GLBuffer::MapWrite|GLBuffer::MapFlushExplicit
    );

    memcpy(string_buf_tex_mapping.get(), topaz_test_string, sizeof(topaz_test_string));
  }

  constexpr size_t num_strings = 2;
  GLBufferTexture string_xy_off_len_buf_tex;
  string_xy_off_len_buf_tex
    .alloc(
        sizeof(i16)*4*num_strings, GLBuffer::DynamicRead, GLBuffer::MapWrite|GLBuffer::ClientStorage
    );
  {
    auto string_xy_off_len_buf_tex_mapping = string_xy_off_len_buf_tex.map(
        GLBuffer::MapWrite|GLBuffer::MapFlushExplicit
    );

    const i16 xy_offsets_lengths[] = { 10, 10, 0, 13, 256, 50, 13, 14 };
    memcpy(string_xy_off_len_buf_tex_mapping.get(), xy_offsets_lengths, sizeof(xy_offsets_lengths));
  }

  GLTextureBuffer string_tex_buf;
  string_tex_buf
    .buffer(r8i, string_buf_tex);

  GLTextureBuffer string_xy_off_len_tex_buf;
  string_xy_off_len_tex_buf
    .buffer(rgba16i, string_xy_off_len_buf_tex);

  gl_context
    .texImageUnit(1)
    .bind(string_tex_buf);

  gl_context
    .texImageUnit(2)
    .bind(string_xy_off_len_tex_buf);

  gl_program
    .uniform("usFontTopaz", gl_context.texImageUnit(0))
    .uniformVec("uvFontColor", 0.5f, 1.0f, 0.2f)
    .uniform("usStrings", gl_context.texImageUnit(1))
    .uniform("usStringXYPositionsOffsetsLengths", gl_context.texImageUnit(2));

  auto topaz_1bpp = load_font("Topaz.raw");
  if(!topaz_1bpp) {
    puts("couldn't load font file `Topaz.raw'!");
    return -1;
  }

  auto topaz = OSDBitmapFont().loadBitmap1bpp(topaz_1bpp->data(), topaz_1bpp->size());
  printf("topaz_1bpp.size()=%zu  topaz.size()=%zu\n", topaz_1bpp->size(), topaz.pixelDataSize());

  auto c = x11().connection<xcb_connection_t>();

  auto topaz_tex_pixel_buf = GLPixelBuffer(GLPixelBuffer::Upload);

  topaz_tex_pixel_buf
    .alloc(topaz.pixelDataSize(), GLBuffer::StaticRead, topaz.pixelData());

  GLTexture2D topaz_tex;

  topaz_tex
    .alloc(8, 4096, 1, r8);

  topaz_tex_pixel_buf
    .uploadTexture(topaz_tex, 0, r, GLType::u8);

  GLFence topaz_tex_uploaded;
  topaz_tex_uploaded
    .fence()
    .sync();

  printf("topaz_tex_pixel_buf.signaled=%d\n\n", topaz_tex_uploaded.signaled());

  GLSampler topaz_tex_sampler;
  topaz_tex_sampler
    .iParam(GLSampler::WrapS, GLSampler::Repeat)
    .iParam(GLSampler::WrapT, GLSampler::Repeat)
    .iParam(GLSampler::MinFilter, GLSampler::Nearset)
    .iParam(GLSampler::MagFilter, GLSampler::Nearset);

  gl_context.texImageUnit(0)
    .bind(topaz_tex, topaz_tex_sampler);

  u16 string_vert_indices[14*5];
  for(size_t i = 0; i < sizeof(string_vert_indices)/sizeof(u16); i++) {
    u16 idx = (i % 5) < 4 ? (i % 5)+(i/5)*4 : 0xFFFF;

    string_vert_indices[i] = idx;
  }

  GLIndexBuffer text_index_buf;
  text_index_buf
    .alloc(sizeof(string_vert_indices), GLBuffer::DynamicDraw, string_vert_indices);

  glEnable(GL_PRIMITIVE_RESTART);
  glPrimitiveRestartIndex(0xFFFF);

  gl_program
    .use();

  GLVertexFormat color_vertex_format;

  constexpr GLSize ColorsDataSize = 4*(3+1) /* size of the array */ * sizeof(uint8_t);
  const uint8_t ColorsData[
      4 /* num vertices */ * (3 /* num components (R,G,B) per vertex */ + 1 /* padding */)
  ] = {
      0xFF, 0x00, 0x00, /* padding byte */ 0x00,
      0xFF, 0xFF, 0x00, /* padding byte */ 0x00,
      0x00, 0x80, 0xFF, /* padding byte */ 0x00,
      0x80, 0x00, 0xCC, /* padding byte */ 0x00,
  };

  GLVertexBuffer color_data_buf;
  color_data_buf
    .alloc(ColorsDataSize, GLBuffer::DynamicDraw, GLBuffer::MapWrite);

  if(auto color_data_mapping = color_data_buf.map(GLBuffer::MapWrite)) {
    memcpy(color_data_mapping.get(), ColorsData, ColorsDataSize);
  }

  printf("(empty)color_vertex_format.vertexByteSize()=%d\n\n", color_vertex_format.vertexByteSize());

  color_vertex_format
    .attr(0, 3, GLType::u8, 0)
    .padding(sizeof(uint8_t));   // Pad data for vertices on 4-byte boundaries

  printf("color_vertex_format.vertexByteSize()=%d\n\n", color_vertex_format.vertexByteSize());

  auto vertex_array = color_vertex_format
    .bindVertexBuffer(0, color_data_buf)
    .createVertexArray();

  vertex_array
    .bind();

  topaz_tex_uploaded
    .block(1);
  printf("topaz_tex_pixel_buf.signaled=%d\n\n", topaz_tex_uploaded.signaled());
  
  bool running = true;
  bool change = false;
  while(auto ev = event_loop.event(IEventLoop::Block)) {
    switch(ev->type()) {
    case Event::KeyDown: {
      auto event = (IKeyEvent *)ev.get();

      auto code = event->code();
      auto sym  = event->sym();

      printf("code=(0x%2X %3u, %c) sym=(0x%2X %3u, %c)\n",
          code, code, code, sym, sym, sym);

      if(sym == 'q') running = false;

      if(sym == 'c' || change) {
        const char new_text[] = "hello - again!!";

        auto string_buf_tex_mapping = string_buf_tex.map(GLBuffer::MapWrite);

        memcpy(string_buf_tex_mapping.get(), new_text, sizeof(new_text));
        change = false;
      }
      break;
    }

    case Event::MouseMove: {
      auto event = (IMouseEvent *)ev.get();
      auto delta = event->delta();

      break;
    }

    case Event::MouseDown: {
      auto event = (IMouseEvent *)ev.get();
      auto point = event->point();
      auto delta = event->delta();

      printf("click! @ (%hd, %hd) delta=(%hd, %hd)\n",
          point.x, point.y, delta.x, delta.y);

      window.drawString("hello, world!",
          Geometry::xy(point.x, point.y), Color::white()); 

      break;
    }

    case Event::Quit:
      running = false;
      break;
    }

    if(change) {
      const char new_text[] = "hello - again!!";

      auto string_buf_tex_mapping = string_buf_tex.map(GLBuffer::MapWrite);

      memcpy(string_buf_tex_mapping.get(), new_text, sizeof(new_text));
      change = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_index_buf.id());
    glDrawElementsInstanced(
        GL_TRIANGLE_FAN, 14*5, GL_UNSIGNED_SHORT, (const void *)0, 2
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

/*
    for(int i = 0; i < 32; i++) {
      float x1 = ((48.0f * (float)i) / 960.0f)-1.0f;
      float x2 = ((48.0f * (float)(i+1)) / 960.0f)-1.0f;
      float v1 = (float)i * (128.0f/4096.0f);
      float v2 = (float)(i+1) * (128.0f/4096.0f);

      //printf("x1=%f x2=%f v1=%f v2=%f\n", x1, x2, v1, v2);

      glBegin(GL_TRIANGLE_FAN);

      //glColor3f(0.0f, 1.0f, 0.0f);
      glTexCoord2f(0.0f, v1);
      glVertex2f(x1, 1.0f);

      //glColor3f(0.0f, 0.0f, 1.0f);
      glTexCoord2f(0.0f, v2);
      glVertex2f(x1, -1.0f);

      //glColor3f(1.0f, 1.0f, 0.0f);
      glTexCoord2f(1.0f, v2);
      glVertex2f(x2, -1.0f);

      //glColor3f(0.0f, 1.0f, 1.0f);
      glTexCoord2f(1.0f, v1);
      glVertex2f(x2, 1.0f);

      glEnd();
    }
    */

    gl_context.swapBuffers();

    if(!running) break;
  }

  gl_context
    .destroy();

  window
     .destroy();

  x11_finalize();

  return 0;
}
