#include <gx/program.h>

#include <GL/gl3w.h>

#include <cassert>

#include <utility>

namespace brdrive {

auto Type_to_shaderType(GLShader::Type type) -> GLenum
{
  switch(type) {
  case GLShader::Vertex:   return GL_VERTEX_SHADER;
  case GLShader::Geometry: return GL_GEOMETRY_SHADER;
  case GLShader::Fragment: return GL_FRAGMENT_SHADER;
  }

  return ~0u;
}

GLShader::GLShader(Type type) :
  type_(Type_to_shaderType(type)),
  id_(GLNullObject),
  compiled_(false)
{
}

GLShader::GLShader(GLShader&& other) :
  type_(other.type_)
{
  std::swap(id_, other.id_);
}

GLShader::~GLShader()
{
  if(id_ == GLNullObject) return;

  glDeleteShader(id_);
}

auto GLShader::id() const -> GLObject
{
  return id_;
}

auto GLShader::source(std::string_view src) -> GLShader&
{
  sources_.emplace_back(std::move(src));

  return *this;
}

auto GLShader::compile() -> GLShader&
{
  assert(!sources_.empty() &&
      "attempted to compile() a GLShader with no sources attached!");

  // Lazily allocate the shader object
  id_ = glCreateShader(type_);

  auto num_sources = (GLsizei)sources_.size();

  std::vector<const GLchar *> source_strings;
  std::vector<int> source_lengths;

  source_strings.reserve(sources_.size());
  source_lengths.reserve(sources_.size());
  for(const auto& v : sources_) {
    source_strings.push_back((const GLchar *)v.data());
    source_lengths.push_back((int)v.size());
  }

  glShaderSource(id_, num_sources, source_strings.data(), source_lengths.data());
  assert(glGetError() == GL_NO_ERROR);

  // glShaderSource makes it's own internal copy of the strings
  //   so their memory can be freed right after the call returns
  sources_.clear();

  glCompileShader(id_);

  int compile_successful = -1;
  glGetShaderiv(id_, GL_COMPILE_STATUS, &compile_successful);

  // Check if there was an error during shader compilation...
  if(compile_successful != GL_TRUE) throw CompileError();

  // ...and if there wasn't mark the shader as compiled
  compiled_ = true;

  return *this;
}

auto GLShader::compiled() const -> bool
{
  return compiled_;
}

auto GLShader::infoLog() const -> std::optional<std::string>
{
  assert(id_ != GLNullObject);

  int info_log_length = -1;
  glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &info_log_length);

  assert(info_log_length >= 0);

  if(!info_log_length) return std::nullopt;

  std::string info_log(info_log_length, 0);
  glGetShaderInfoLog(id_, info_log.length(), nullptr, info_log.data());

  return std::move(info_log);
}

GLProgram::GLProgram() :
  id_(GLNullObject)
{
}

GLProgram::GLProgram(GLProgram&& other) :
  GLProgram()
{
  std::swap(id_, other.id_);
}

GLProgram::~GLProgram()
{
  if(id_ == GLNullObject) return;

  glDeleteProgram(id_);
}

auto GLProgram::attach(const GLShader& shader) -> GLProgram&
{
  assert(shader.id() != GLNullObject && "attempted to attach() a null GLShader!");
  assert(shader.compiled() &&
      "attempted to attach() a GLShader which hadn't yet been compiled!");

  // Lazily allocate the program object
  if(id_ == GLNullObject) id_ = glCreateProgram();

  glAttachShader(id_, shader.id());

  assert(glGetError() != GL_INVALID_OPERATION &&
      "attempted to attach() a GLShader that's already attached!");

  return *this;
}

auto GLProgram::detach(const GLShader& shader) -> GLProgram&
{
  assert(id_ != GLNullObject);
  assert(shader.id() != GLNullObject && "attempted to detach() a null GLShader!");

  glDetachShader(id_, shader.id());

  assert(glGetError() != GL_INVALID_OPERATION &&
      "attempted to detach() a GLShader not attached to this GLProgram!");

  return *this;
}

auto GLProgram::link() -> GLProgram&
{
  assert(id_ != GLNullObject);

  glLinkProgram(id_);

  int link_successful = -1;
  glGetProgramiv(id_, GL_LINK_STATUS, &link_successful);

  // Check if there was an error during linking the program
  if(link_successful != GL_TRUE) throw LinkError();

  return *this;
}

auto GLProgram::infoLog() const -> std::optional<std::string>
{
  assert(id_ != GLNullObject);

  int info_log_length = -1;
  glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &info_log_length);

  assert(info_log_length >= 0);

  if(!info_log_length) std::nullopt;

  std::string info_log(info_log_length, 0);
  glGetProgramInfoLog(id_, info_log.size(), nullptr, info_log.data());

  return std::move(info_log);
}

}

