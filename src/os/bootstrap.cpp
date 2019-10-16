#include <os/bootstrap.h>
#include <os/dl.h>

#if defined(__linux__)
#  include <dlfcn.h>
#else
#  error "unsupported OS!"
#endif

#include <cstdio>

namespace brdrive {

auto bootstrap_os_dl() -> DynamicLibrary
{
  void *dl_handle = nullptr;

#if defined(__linux__)
  // Clear error status
  dlerror();

  dl_handle = dlopen("./libBrunerDrive_sys.so", RTLD_NOW | RTLD_GLOBAL);

  if(!dl_handle) {
    puts(dlerror());

    return DynamicLibrary();
  }
#endif

  return DynamicLibrary::from_handle(
      DynamicLibrary::FromHandleFriendKey(), dl_handle
  );
}

}
