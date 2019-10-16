set (SrcDir ${PROJECT_SOURCE_DIR}/src)

target_sources (BrunerDrive PRIVATE
  ${SrcDir}/brdrive.cpp

  # OS specific sources
  ${SrcDir}/os/interface.cpp
  ${SrcDir}/os/dl.cpp
  ${SrcDir}/os/bootstrap.cpp

  # OpenGL
  ${SrcDir}/gx/gx.cpp
  ${SrcDir}/gx/context.cpp
  ${SrcDir}/gx/extensions.cpp
  ${SrcDir}/gx/pipeline.cpp
  ${SrcDir}/gx/vertex.cpp
  ${SrcDir}/gx/buffer.cpp
  ${SrcDir}/gx/texture.cpp
  ${SrcDir}/gx/program.cpp
  ${SrcDir}/gx/fence.cpp
  ${SrcDir}/gx/handle.cpp

  # OSD specific sources
  ${SrcDir}/osd/osd.cpp
  ${SrcDir}/osd/util.cpp
  ${SrcDir}/osd/font.cpp
  ${SrcDir}/osd/shaders.cpp
  ${SrcDir}/osd/drawcall.cpp
  ${SrcDir}/osd/surface.cpp
)

target_sources (BrunerDrive_sys PRIVATE

  # Window specific sources
  ${SrcDir}/window/window.cpp
  ${SrcDir}/window/event.cpp

  # OpenGL/GLX
  ${SrcDir}/gx/gx.cpp
  ${SrcDir}/gx/context.cpp
  ${SrcDir}/gx/extensions.cpp
  ${SrcDir}/gx/pipeline.cpp
  ${SrcDir}/gx/vertex.cpp
  ${SrcDir}/gx/buffer.cpp
  ${SrcDir}/gx/texture.cpp
  ${SrcDir}/gx/program.cpp
  ${SrcDir}/gx/fence.cpp
  ${SrcDir}/gx/handle.cpp

  # X11 specific sources
  ${SrcDir}/x11/x11.cpp
  ${SrcDir}/x11/interface.cpp
  ${SrcDir}/x11/connection.cpp
  ${SrcDir}/x11/window.cpp
  ${SrcDir}/x11/event.cpp
  ${SrcDir}/x11/glx.cpp

)
