#pragma once

namespace brdrive {

class ISysInterface {
public:
  using InitFn = void (*)();
  using FinalizeFn = void (*)();

  virtual auto init() -> ISysInterface&;
  virtual auto finalize() -> ISysInterface&;

protected:
  auto implFactoriesPtr() -> void *;

private:
};

template <typename T>
class SysInterface : ISysInterface {
public:
  auto implFactories() -> T *
  {
    return (T *)implFactoriesPtr();
  }
};

}
