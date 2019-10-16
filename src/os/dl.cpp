#include <os/dl.h>

#if defined(__linux__)
#  include <dlfcn.h>
#else
#  error "unsupported OS!"
#endif

#include <cassert>
#include <cstdio>

#include <utility>

namespace brdrive {

DynamicLibrary::DynamicLibrary() :
  handle_(nullptr)
{
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) :
  DynamicLibrary()
{
  std::swap(handle_, other.handle_);
}

auto DynamicLibrary::interface(const char *token) -> ISysInterface *
{
  assert(handle_ &&
      "attempted to get an interface() from a null DynamicLibrary!");

  ISysInterface *interface = nullptr;

#if defined(__linux__)
  void *dl_x11_init = dlsym(handle_, "brdriveISys_x11_init");
  void *dl_x11_finalize = dlsym(dl_handle, "brdriveISys_x11_finalize");
  void *dl_x11_impl = dlsym(dl_handle, "brdriveISys_x11_impl");

#endif

  return interface;
}

auto DynamicLibrary::from_handle(FromHandleFriendKey, void *handle) -> DynamicLibrary
{
  DynamicLibrary dl;

  dl.handle_ = handle;

  return std::move(dl);
}

}
