#pragma once

namespace brdrive {

// Forward declarations
class GLVertexArray;
class GLVertexArrayHandle;

namespace vertex_format_detail {
auto GLVertexArray_to_ptr(GLVertexArray&& array) -> GLVertexArrayHandle;
}

// Wrapper around heap allocated GLVertexArray instance
class GLVertexArrayHandle {
public:
  GLVertexArrayHandle();
  GLVertexArrayHandle(const GLVertexArrayHandle&) = delete;
  GLVertexArrayHandle(GLVertexArrayHandle&& other);
  ~GLVertexArrayHandle();

  auto operator=(GLVertexArrayHandle&& other) -> GLVertexArrayHandle&;

  auto get() -> GLVertexArray*;
  auto get() const -> const GLVertexArray*;

  operator bool() const { return get(); }

  auto operator->() -> GLVertexArray* { return get(); }

/*
semi-private:
*/
  class ConstructorFriendKey {
    ConstructorFriendKey() { }

    friend auto vertex_format_detail::GLVertexArray_to_ptr(
        GLVertexArray&& array
      ) -> GLVertexArrayHandle;
  };

  GLVertexArrayHandle(ConstructorFriendKey, GLVertexArray *ptr);

private:
  GLVertexArray *ptr_;
};

}