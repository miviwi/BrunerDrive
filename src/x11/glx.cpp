#include <x11/glx.h>
#include <x11/x11.h>
#include <x11/connection.h>
#include <x11/window.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

// OpenGL headers
#include <GL/gl.h>

namespace brdrive {

static int GLX_VisualAttribs[] = {
  GLX_X_RENDERABLE, True,
  GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,

  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,

  GLX_RENDER_TYPE, GLX_RGBA_BIT,
  GLX_RED_SIZE,   8,
  GLX_GREEN_SIZE, 8,
  GLX_BLUE_SIZE,  8,
  GLX_ALPHA_SIZE, 8,

  GLX_DOUBLEBUFFER, True,

  None,
};

struct pGLXContext {
  Display *display;

  ::GLXContext context = nullptr;
  GLXWindow window = 0;

  ~pGLXContext();
};

pGLXContext::~pGLXContext()
{
  auto display = x11().xlibDisplay<Display>();

  if(window) glXDestroyWindow(display, window);
  if(context) glXDestroyContext(display, context);
}

GLXContext::GLXContext() :
  p(nullptr)
{
}

GLXContext::~GLXContext()
{
  delete p;
}

auto GLXContext::acquire(IWindow *window_, GLContext *share) -> GLContext&
{
  assert(brdrive::x11_was_init()
      && "x11_init() MUST be called prior to creating a GLXContext!");

  auto display = x11().xlibDisplay<Display>();

  int num_fb_configs = 0;
  auto fb_configs = glXChooseFBConfig(
      display, x11().defaultScreen(),
      GLX_VisualAttribs, &num_fb_configs
  );
  if(!fb_configs || !num_fb_configs) throw NoSuitableFramebufferConfigError();

  int visual_id = ~0;
  auto fb_config = fb_configs[0];
  glXGetFBConfigAttrib(
      display, fb_config, GLX_VISUAL_ID, &visual_id
  );

  p = new pGLXContext();
  p->display = display;

  p->context = glXCreateNewContext(
      display, fb_config, GLX_RGBA_TYPE,
      /* shareList */ share ? (::GLXContext)share->handle() : nullptr,
      /* direct */ True
  );
  if(!p->context) throw AcquireError();

  auto window = (X11Window *)window_;
  auto x11_window_handle = window->windowHandle();

  p->window = glXCreateWindow(
      display, fb_config, x11_window_handle, nullptr
  );
  if(!p->window) {
    glXDestroyContext(display, p->context);

    delete p;
    p = nullptr;

    throw AcquireError();
  }

  return *this;
}

auto GLXContext::makeCurrent() -> GLContext&
{
  assert(p && "the context must've been acquire()'d to makeCurrent()!");

  auto display  = p->display;
  auto drawable = (GLXDrawable)p->window;
  auto context  = p->context;

  auto success = glXMakeContextCurrent(display, drawable, drawable, context);
  if(!success) throw AcquireError();

  return *this;
}

auto GLXContext::swapBuffers() -> GLContext&
{
  auto drawable = (GLXDrawable)p->window;
  glXSwapBuffers(p->display, drawable);

  return *this;
}

auto GLXContext::destroy() -> GLContext&
{
  delete p;
  p = nullptr;

  return *this;
}

auto GLXContext::handle() -> GLContextHandle
{
  if(!p) return nullptr;

  return p->context;
}

}

