#pragma once

namespace brdrive {

// Forward declarations
class ISysInterface;

class DynamicLibrary {
public:
  DynamicLibrary();
  DynamicLibrary(DynamicLibrary&& other);

  auto interface(const char *token) -> ISysInterface *;

/*
semi-private:
*/
  class FromHandleFriendKey {
    FromHandleFriendKey() { }

    friend auto bootstrap_os_dl() -> DynamicLibrary;
  };

  static auto from_handle(FromHandleFriendKey, void *handle) -> DynamicLibrary;

private:

  void *handle_;
};

}
