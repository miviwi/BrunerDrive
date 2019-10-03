#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

// Forward declarations
class GLContext;
class GLTexture;
class GLTexture2D;
class GLSampler;
class GLTexImageUnit;
class GLBuffer;

class GLTexture {
public:
  enum Dimensions {
    DimensionsInvalid,

    TexImage1D,  // GL_TEXTURE_1D
    TexImage2D,  // GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_*
    TexImage3D,  // GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D
  };

  struct InvalidFormatTypeError : public std::runtime_error {
    InvalidFormatTypeError() :
      std::runtime_error("invalid format (must be untyped) or format/type combination!")
    { }
  };

  GLTexture(const GLTexture&) = delete;
  virtual ~GLTexture();

//  auto operator=(GLTexture&& other) -> GLTexture&;

  auto id() const -> GLObject;

  // Returns a value which informs which of the glTextureSubImage*
  //   functions need to be used for this texture
  auto dimensions() const -> Dimensions;
  // Returns the raw GL_TEXTURE_* value to be passed to OpenGL
  //   functions - glBindTexture(), glTexImage()...
  auto bindTarget() const -> GLEnum;

  auto width() const -> unsigned;
  auto height() const -> unsigned;
  auto depth() const -> unsigned;

protected:
  GLTexture(GLEnum bind_target);

  GLObject id_;

  Dimensions dimensions_;
  GLEnum bind_target_;

  // Initialized to 1 by default so only
  //   the appropriate dimensions need to
  //   be set by the concrete texture types
  unsigned width_, height_, depth_;

  unsigned levels_;
};

class GLTexture2D : public GLTexture {
public:
  GLTexture2D();
  GLTexture2D(GLTexture2D&& other);

  auto alloc(unsigned width, unsigned height, unsigned levels, GLFormat internalformat) -> GLTexture2D&;
  auto upload(unsigned level, GLFormat format, GLType type, const void *data) -> GLTexture2D&;
};

class GLTextureBuffer : public GLTexture {
public:
  GLTextureBuffer();
  GLTextureBuffer(GLTextureBuffer&&) = delete;

  auto buffer(GLFormat internalformat, const GLBuffer& buffer) -> GLTextureBuffer&;

private:
};

class GLSampler {
public:
  enum ParamName {
    WrapS, WrapT, WrapR,
    MinFilter, MagFilter,
    MinLOD, MaxLOD, LODBias,
    CompareMode, CompareFunc,
    SeamlessCubemap,
    MaxAnisotropy,
  };

  enum SymbolicValue : int {
    // Values for Wrap[S,T,R]
    ClampEdge, ClampBorder, Repeat,

    // Values for [Min,Mag]Filter
    //   - MagFilter only allows { Nearest, Linear}
    Nearset, Linear, BiLinear, TriLinear,
    NearestMipmapNearest, NearestMipmapLinear,

    // Values for CompareMode
    None, CompareRefToTex,
  
    // Values for CompareFunc
    Eq, NotEq, Less, LessEq, Greater, GreaterEq, Always, Never,
  };

  struct InvalidParamNameError : public std::runtime_error {
    InvalidParamNameError() :
      std::runtime_error("the 'pname' argument to the [i,f]Param methods must be a value"
          " contained in GLSampler::ParamName!")
    { }
  };

  struct RequiresSymbolicValueError : public std::runtime_error {
    RequiresSymbolicValueError() :
      std::runtime_error("this [i,f]Param requires a GLSampler::SymbolicValue 'value' argument!")
    { }
  };

  struct InvalidSymbolicValueError : public std::runtime_error {
    InvalidSymbolicValueError() :
      std::runtime_error("for [i,f]Param()'s which require a SymbolicValue argument"
          " only then ones contained in GLSampler::SymbolicValue can be used!")
    { }
  };

  GLSampler();
  GLSampler(const GLSampler&) = delete;
  GLSampler(GLSampler&& other);
  ~GLSampler();

//  auto operator=(GLSampler&& other) -> GLSampler&;

  auto id() const -> GLObject;

  auto iParam(ParamName pname, int value) -> GLSampler&;
  auto fParam(ParamName pname, float value) -> GLSampler&;

private:
  void initGLObject();

  GLObject id_;
};

class GLTexImageUnit {
public:
  auto bind(const GLTexture& tex) -> GLTexImageUnit&;
  auto bind(const GLSampler& sampler) -> GLTexImageUnit&;
  auto bind(const GLTexture& tex, const GLSampler& sampler) -> GLTexImageUnit&;

  auto texImageUnitIndex() const -> unsigned;

  auto boundTexture() const -> GLObject;

private:
  friend GLContext;

  GLTexImageUnit(GLContext *context, unsigned slot);

  GLContext *context_;

  unsigned slot_;

  GLObject bound_texture_;
  GLObject bound_sampler_;
};

}
