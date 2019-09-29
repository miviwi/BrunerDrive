#include <gx/program.h>
#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

#include <regex>
#include <utility>

namespace brdrive {

thread_local GLObject g_bound_program = GLNullObject;

[[using gnu: always_inline]]
constexpr auto Type_to_shaderType(GLShader::Type type) -> GLenum
{
  switch(type) {
  case GLShader::Vertex:         return GL_VERTEX_SHADER;
  case GLShader::TessControl:    return GL_TESS_CONTROL_SHADER;
  case GLShader::TessEvaluation: return GL_TESS_EVALUATION_SHADER;
  case GLShader::Geometry:       return GL_GEOMETRY_SHADER;
  case GLShader::Fragment:       return GL_FRAGMENT_SHADER;
  case GLShader::Compute:        return GL_COMPUTE_SHADER;
  }

  return ~0u;
}

static auto shaderType_supported(GLShader::Type type) -> bool
{
  switch(type) {
  // The shader types below are always supported
  //   under the chosen minimum required GL version
  case GLShader::Vertex:       // Fallthrough
  case GLShader::Geometry:
  case GLShader::Fragment:    return true;

  // While the rest must be used via extensions - if desired

  case GLShader::TessControl:      // Fallthrough
  case GLShader::TessEvaluation: return ARB::tessellation_shader;

  case GLShader::Compute: return ARB::compute_shader;
  }

  return false;    // Unreachable
}

GLShader::GLShader(Type type) :
  type_(Type_to_shaderType(type)),
  id_(GLNullObject),
  compiled_(false),
  sources_state_flags_(0),
  version_(DefaultGLSLVersion),
  defines_(std::nullopt)
{

}

GLShader::GLShader(GLShader&& other) :
  type_(other.type_)
{
  std::swap(id_, other.id_);
  std::swap(compiled_, other.compiled_);
  std::swap(sources_state_flags_, other.sources_state_flags_);
  std::swap(version_, other.version_);
  std::swap(defines_, other.defines_);
  std::swap(sources_, other.sources_);
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

auto GLShader::glslVersion(int ver) -> GLShader&
{
  // Check if the version is being set for the first time
  auto version_state = (sources_state_flags_ & VersionStateMask) >> VersionStateShift;

  assert((version_state == VersionStateUseDefault
        || version_state == VersionStateInhibitDefault
        || version_state == VersionStateVersionGiven) &&
      "'sources_state_flags_' contains invalid data!");

  if(version_state != VersionStateUseDefault) throw GLSLVersionRedefinitionError();

  // Now store it...
  version_ = ver;

  // ...and modify the 'sources_state_flags_' appropriately
  u32 new_version_state = ~0u;
  if(ver < 0) {
    new_version_state = (VersionStateInhibitDefault << VersionStateShift);
  } else {
    new_version_state = (VersionStateVersionGiven << VersionStateShift);
  }

  // Clear old version state data...
  sources_state_flags_ &= ~VersionStateMask;
  // ...and fill it with 'new_version_state'
  sources_state_flags_ |= new_version_state&VersionStateMask;

  return *this;
}

auto GLShader::source(std::string_view src) -> GLShader&
{
  sources_.emplace_back(std::move(src));

  // After source text is added - 'sources_state_flags_'
  //   needs to reflect that define() can no longer
  //   be used to add text using push_back()
  constexpr u32 new_defines_state = (DefinesStateInsertingRequired << DefinesStateShift);
  sources_state_flags_ |= (new_defines_state & DefinesStateMask);

  return *this;
}

static const std::regex g_define_identifier_re("^[a-zA-Z_]([a-zA-Z0-9_])*$", std::regex::optimize);
auto GLShader::define(const char *identifier, const char *value) -> GLShader&
{
  // Validate the identifier
  if(!std::regex_match(identifier, g_define_identifier_re)) throw InvalidDefineIdentifierError();

  char define_text_buf[256];
  int num_printed = snprintf(define_text_buf, sizeof(define_text_buf),
      "#define %s %s\n",
      identifier, value ? value : ""
  );

  assert((num_printed > 0 && num_printed < sizeof(define_text_buf)) &&
      "buffer for sprintf() in GLShader::define() is too small!");

  std::string define_text(define_text_buf, num_printed);

  auto& defines_vector = ([this]() -> std::vector<std::string>&
  {
    // Create the std::vector
    if(!defines_) defines_.emplace();

    return defines_.value();
  })();

  u32 defines_state = (sources_state_flags_ & DefinesStateMask) >> DefinesStateShift;
  if(defines_state != DefinesStateInsertingRequired) {
    // No sources were added so push_back() can be used
    //   to append the text, which saves on performance
    defines_vector.emplace_back(std::move(define_text));
  } else {     // Have to use insert
    defines_vector.emplace(defines_vector.begin(), std::move(define_text));
  }

  return *this;
}

auto GLShader::compile() -> GLShader&
{
  assert(!sources_.empty() &&
      "attempted to compile() a GLShader with no sources attached!");

  // Lazily allocate the shader object
  id_ = glCreateShader(type_);

  auto version_string_state = (sources_state_flags_ & VersionStateMask) >> VersionStateShift;
  GLSize has_version_string = (version_string_state == VersionStateInhibitDefault) ? 0 : 1;

  GLSize num_defines = defines_.has_value() ? (GLSize)defines_->size() : 0;

  auto num_sources = has_version_string + num_defines + (GLSize)sources_.size();

  std::vector<const GLchar *> source_strings;
  std::vector<int> source_lengths;

  source_strings.reserve(num_sources);
  source_lengths.reserve(num_sources);

  // First add the #version string (if needed)
  std::string version_string;   // Define here so it doesn't go out of scope prematurely
  if(has_version_string) {
    char version_string_buf[64];
    int num_printed = snprintf(version_string_buf, sizeof(version_string_buf),
        "#version %d\n\n",
        version_);

    assert((num_printed > 0 && num_printed < sizeof(version_string_buf)) &&
        "the sprintf() for the #version directive didn't fit in the buffer!");

    version_string.assign(version_string_buf, num_printed);

    source_strings.push_back((const GLchar *)version_string.data());
    source_lengths.push_back((int)version_string.size());
  }

  // Next - the #defines
  for(size_t i = 0; i < num_defines; i++) {
    const auto& define_string = defines_->at(i);

    source_strings.push_back((const GLchar *)define_string.data());
    source_lengths.push_back((int)define_string.size());
  }

  // And lastly the sources
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
  if(id_ == GLNullObject) return std::nullopt;

  int info_log_length = -1;
  glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &info_log_length);

  assert(info_log_length >= 0);

  if(!info_log_length) return std::nullopt;

  std::string info_log(info_log_length, 0);
  glGetShaderInfoLog(id_, info_log.length(), nullptr, info_log.data());

  return std::move(info_log);
}

GLProgram::GLProgram() :
  id_(GLNullObject),
  linked_(false)
{
}

GLProgram::GLProgram(GLProgram&& other) :
  GLProgram()
{
  std::swap(id_, other.id_);
  std::swap(linked_, other.linked_);
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

  linked_ = true;

  return *this;
}

auto GLProgram::infoLog() const -> std::optional<std::string>
{
  if(id_ == GLNullObject) return std::nullopt;

  int info_log_length = -1;
  glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &info_log_length);

  assert(info_log_length >= 0);

  if(!info_log_length) std::nullopt;

  std::string info_log(info_log_length, 0);
  glGetProgramInfoLog(id_, info_log.size(), nullptr, info_log.data());
  
  assert(glGetError() == GL_NO_ERROR);

  return std::move(info_log);
}

auto GLProgram::use() -> GLProgram&
{
  assert(linked_ &&
    "attempted to use() a GLProgram which hasn't been link()'ed!");

  // Only switch the program if it's not the same as the bound one
  if(id_ == g_bound_program) return *this;

  glUseProgram(id_);
  g_bound_program = id_;

  return *this;
}

auto GLProgram::uniform(const char *name, int i) -> GLProgram&
{
  assert(id_ != GLNullObject);
  assert(linked_ &&
    "attempted to upload a uniform to a GLProgram which hasn't been link()'ed!");

  UniformLocation location = InvalidLocation;
  auto location_it = uniforms_.find(name);
  if(location_it == uniforms_.end()) {
    location = glGetUniformLocation(id_, name);

    uniforms_.emplace(name, location);
  } else {
    location = location_it->second;
  }

  // Avoid causing OpenGL errors
  if(location == InvalidLocation) return *this;

  // Use direct state access if it's available...
  if(ARB::direct_state_access() || EXT::direct_state_access()) {
    glProgramUniform1i(id_, location, i);
  } else {     // ...and fall back to the old path otherwise
    glUseProgram(id_);

    glUniform1i(id_, i);
  }

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

}

