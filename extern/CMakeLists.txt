set (ExternDir ${PROJECT_SOURCE_DIR}/extern)

target_sources (BrunerDrive PUBLIC
  # OpenGL/gl3w
  ${ExternDir}/GL/gl3w.c
)

target_sources (BrunerDrive_sys PUBLIC
  # OpenGL/gl3w
  ${ExternDir}/GL/gl3w.c
)
