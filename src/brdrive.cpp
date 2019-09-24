#include <cstdio>
#include <cstdlib>

#include <brdrive.h>
#include <types.h>
#include <window/geometry.h>
#include <window/color.h>
#include <window/window.h>
#include <gx/gx.h>
#include <gx/extensions.h>
#include <gx/context.h>
#include <gx/buffer.h>
#include <gx/texture.h>
#include <gx/program.h>
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

auto load_font(const std::string& file_name) -> std::optional<std::vector<brdrive::u8>>
{
  auto fd = open(file_name.data(), O_RDONLY);
  if(fd < 0) return std::nullopt;

  struct stat st;
  if(fstat(fd, &st) < 0) return std::nullopt;

  std::vector<brdrive::u8> font(st.st_size);
  if(read(fd, font.data(), font.size()) < 0) {
    close(fd);
    return std::nullopt;
  }

  close(fd);
  return std::move(font);
}

auto expand_1bpp_to_8bpp(const std::vector<brdrive::u8>& font) -> std::vector<brdrive::u8>
{
  std::vector<brdrive::u8> expanded;
  expanded.reserve(font.size() * 8);

  for(auto b : font) {
    for(int i = 0; i < 8; i++) {
      brdrive::u8 pixel = (b >> 7)*0xFF;

      expanded.push_back(pixel);
      b <<= 1;
    }
  }

  return std::move(expanded);
}

int main(int argc, char *argv[])
{
  brdrive::x11_init();

  brdrive::X11Window window;
  brdrive::X11EventLoop event_loop;

  window
    .geometry({ 0, 0, 960, 512 })
    .background(brdrive::Color(1.0f, 0.0f, 1.0f, 0.0f))
    .create()
    .show();

  event_loop
    .init(&window);

  brdrive::GLXContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent();

  brdrive::gx_init();

  puts(gl_context.versionString().data());

  auto gl_version = gl_context.version();
  printf("OpenGL %d.%d\n", gl_version.major, gl_version.minor);

  printf("ARB_vertex_attrib_binding: %d\n"
    "ARB_separate_shader_objects: %d\n"
    "ARB_direct_state_access: %d\n"
    "EXT_direct_state_access: %d\n\n",
    brdrive::ARB::vertex_attrib_binding(),
    brdrive::ARB::separate_shader_objects(),
    brdrive::ARB::direct_state_access(),
    brdrive::EXT::direct_state_access()
  );

  brdrive::GLShader vert(brdrive::GLShader::Vertex);
  brdrive::GLShader frag(brdrive::GLShader::Fragment);

  vert.source(R"VERT(
#version 330

void main()
{
  switch(gl_VertexID) {
  case 0: gl_Position = vec4(-1.0f, +1.0f, 0.1f, 1.0f); break;
  case 1: gl_Position = vec4(-1.0f, -1.0f, 0.1f, 1.0f); break;
  case 2: gl_Position = vec4(+1.0f, -1.0f, 0.1f, 1.0f); break;
  case 3: gl_Position = vec4(+1.0f, +1.0f, 0.1f, 1.0f); break;
  }
}
)VERT");

  frag.source(R"FRAG(
#version 330

out vec3 oFragColor;

uniform sampler2D usOSD;

void main()
{
  oFragColor = texture(usOSD, gl_FragCoord.st).rgb;
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

  brdrive::GLProgram gl_program;

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

  gl_program
    .uniform("usOSD", 0);

  glEnable(GL_TEXTURE_2D);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glClearColor(1.0f, 1.0f, 0.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT);

  auto topaz_1bpp = load_font("Topaz.raw");
  if(!topaz_1bpp) {
    puts("couldn't load font file `Topaz.raw'!");
    return -1;
  }

  auto topaz = expand_1bpp_to_8bpp(topaz_1bpp.value());
  printf("topaz_1bpp.size()=%zu  topaz.size()=%zu\n", topaz_1bpp->size(), topaz.size());

  auto c = brdrive::x11().connection<xcb_connection_t>();

  brdrive::GLTexture2D topaz_tex;

  topaz_tex
    .alloc(8, 4096, 1, brdrive::r8)
    .upload(0, brdrive::r, brdrive::GLType::u8, topaz.data());

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, topaz_tex.id());

  bool running = true;
  while(auto ev = event_loop.event(brdrive::IEventLoop::Block)) {
    switch(ev->type()) {
    case brdrive::Event::KeyUp: {
      auto event = (brdrive::IKeyEvent *)ev.get();

      auto code = event->code();
      auto sym  = event->sym();

      printf("code=(0x%2X %3u, %c) sym=(0x%2X %3u, %c)\n",
          code, code, code, sym, sym, sym);

      if(sym == 'q') running = false;
      break;
    }

    case brdrive::Event::MouseMove: {
      auto event = (brdrive::IMouseEvent *)ev.get();
      auto delta = event->delta();

      break;
    }

    case brdrive::Event::MouseDown: {
      auto event = (brdrive::IMouseEvent *)ev.get();
      auto point = event->point();
      auto delta = event->delta();

      printf("click! @ (%hd, %hd) delta=(%hd, %hd)\n",
          point.x, point.y, delta.x, delta.y);

      window.drawString("hello, world!",
          brdrive::Geometry::xy(point.x, point.y), brdrive::Color::white()); 

      break;
    }

    case brdrive::Event::Quit:
      running = false;
      break;
    }

/*
    glClear(GL_COLOR_BUFFER_BIT);

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

    gl_context.swapBuffers();
    */

    if(!running) break;
  }

  gl_context
    .destroy();

  window
     .destroy();

  brdrive::x11_finalize();

  return 0;
}
