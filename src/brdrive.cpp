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

auto expand_1bpp_to_8bpp(const std::vector<uint8_t>& font) -> std::vector<uint8_t>
{
  std::vector<uint8_t> expanded;
  expanded.reserve(font.size() * 8);

  for(auto b : font) {
    for(int i = 0; i < 8; i++) {
      uint8_t pixel = (b >> 7)*0xFF;

      expanded.push_back(pixel);
      b <<= 1;
    }
  }

  return std::move(expanded);
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

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  gl_context
    .dbg_EnableMessages();

  printf("OpenGL %s\n\n", gl_context.versionString().data());

  GLShader vert(GLShader::Vertex);
  GLShader frag(GLShader::Fragment);

  vert
    .glslVersion(330)
    .define("DEF_BEFORE_SOURCE")
    .source(R"VERT(
layout(location = 0) in vec3 viColor;

out Vertex {
  vec3 ScreenPosition;
  vec3 Color;
  vec2 UV;
} vo;

const vec4 Positions[4] = vec4[](
  vec4(-1.0f, +1.0f, 0.1f, 1.0f),
  vec4(-1.0f, -1.0f, 0.1f, 1.0f),
  vec4(+1.0f, -1.0f, 0.1f, 1.0f),
  vec4(+1.0f, +1.0f, 0.1f, 1.0f)
);

const vec2 UVs[4] = vec2[](
  vec2(0.0f, 0.0f),
  vec2(0.0f, 1.0f),
  vec2(1.0f, 1.0f),
  vec2(1.0f, 0.0f)
);

const float ScreenAspect = 16.0f/9.0f;

void main()
{
  vec4 pos = Positions[gl_VertexID];
  vec2 uv = UVs[gl_VertexID];
  vec3 screen_pos = vec3(pos.x * ScreenAspect, pos.yz);

  vo.ScreenPosition = screen_pos;
  vo.Color = viColor;
  vo.UV = uv;

  gl_Position = pos;
}
)VERT")
  .define("DEF_AFTER_SOURCE")
  ;

  frag.source(R"FRAG(
in Vertex {
  vec3 ScreenPosition;
  vec3 Color;
  vec2 UV;
} fi;

out vec4 foFragColor;

uniform sampler2D usFontTopaz;

const float Pi = 3.14159;

vec3 circle(float r, vec3 inside, vec3 outside)
{
  vec2 pos = fi.ScreenPosition.xy;

  float d = pos.x*pos.x + pos.y*pos.y;
  float mask = smoothstep(r - 0.005f, r + 0.005f, d);

  return mix(inside, outside, mask);
}

vec3 checkerboard(vec2 offset, vec3 color_a, vec3 color_b)
{
  vec2 arg = fi.ScreenPosition.xy*2.0f*Pi + offset;
  float checkers = sin(arg.x) * cos(arg.y);

  return mix(color_a, color_b, step(0.0f, checkers));
}

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

//  foFragColor = circle(Radius, Yellow, LBlue);

//  foFragColor = checkerboard(vec2(Pi/3.0f, 0.0f), Orange, Blue);
    
    foFragColor = vec4(fi.Color, 1.0f);

//  foFragColor = texture(usFontTopaz, (fi.UV+vec2(0, 7.0f))*vec2(4, 1.0f/32.0f)).rrrr;
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
)COMPUTE");

  gl_program
    .uniform("usFontTopaz", 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  glClearColor(1.0f, 1.0f, 0.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT);

  auto topaz_1bpp = load_font("Topaz.raw");
  if(!topaz_1bpp) {
    puts("couldn't load font file `Topaz.raw'!");
    return -1;
  }

  auto topaz = expand_1bpp_to_8bpp(topaz_1bpp.value());
  printf("topaz_1bpp.size()=%zu  topaz.size()=%zu\n", topaz_1bpp->size(), topaz.size());

  auto c = x11().connection<xcb_connection_t>();

  auto topaz_tex_pixel_buf = GLPixelBuffer(GLPixelBuffer::Upload);

  topaz_tex_pixel_buf
    .alloc(topaz.size(), GLBuffer::StaticRead, topaz.data());

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
  while(auto ev = event_loop.event(IEventLoop::Block)) {
    switch(ev->type()) {
    case Event::KeyUp: {
      auto event = (IKeyEvent *)ev.get();

      auto code = event->code();
      auto sym  = event->sym();

      printf("code=(0x%2X %3u, %c) sym=(0x%2X %3u, %c)\n",
          code, code, code, sym, sym, sym);

      if(sym == 'q') running = false;
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

    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

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
