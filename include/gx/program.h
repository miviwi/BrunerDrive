#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <optional>

namespace brdrive {

class GLShader {
public:
  enum Type {
    Invalid,
    Vertex, Geometry, Fragment,
  };

  struct CompileError : public std::runtime_error {
    CompileError() :
      std::runtime_error("failed to compile() to GLShader!")
    { }
  };

  GLShader(Type type);
  GLShader(const GLShader&) = delete;
  GLShader(GLShader&& other);
  ~GLShader();

  auto id() const -> GLObject;

  auto source(std::string_view src) -> GLShader&;

  // Must be called after appending all the sources
  auto compile() -> GLShader&;

  // Returns 'true' if the GLShader has been
  //   compiled sucessfully
  auto compiled() const -> bool;

  auto infoLog() const -> std::optional<std::string>;

private:
  GLEnum type_;
  GLObject id_;

  bool compiled_;

  std::vector<std::string_view> sources_;
};

class GLProgram {
public:
  using UniformLocation = int;

  enum : UniformLocation {
    InvalidLocation = -1,
  };

  struct LinkError : public std::runtime_error {
    LinkError() :
      std::runtime_error("linking the GLProgram failed!")
    { }
  };

  GLProgram();
  GLProgram(const GLProgram&) = delete;
  GLProgram(GLProgram&& other);
  ~GLProgram();

  auto attach(const GLShader& shader) -> GLProgram&;
  auto detach(const GLShader& shader) -> GLProgram&;

  // - Can be called only AFTER attach()'ing all shaders
  // - Must be called BEFORE the program is bound to the
  //   pipeline
  auto link() -> GLProgram&;
  // Returns 'true' if link() was previously
  //   called (and succeeded) on this program
  auto linked() const -> bool;

  // Returns a string containing error messages
  //   after a LinkError() is thrown
  auto infoLog() const -> std::optional<std::string>;

  // Bind the program to the pipeline
  //   - Can ONLY be called if linked() == true
  auto use() -> GLProgram&;
  
  auto uniform(const char *name, int i) -> GLProgram&;

private:
  GLObject id_;
  bool linked_;

  // Lazy-initialized when uploading a uniform for the first time
  std::unordered_map<std::string, UniformLocation> uniforms_;
};

}
