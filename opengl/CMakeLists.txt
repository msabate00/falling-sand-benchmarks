cmake_minimum_required(VERSION 3.16)
project(FallingSand VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# === GLFW (FetchContent) ===
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# === GLAD (pre-generado, sin Python) ===
add_library(glad_gl_core_33 STATIC
  third_party/glad2/src/gl.c
)
target_include_directories(glad_gl_core_33 PUBLIC
  third_party/glad2/include
)

# === Ejecutable ===
add_executable(FallingSand
  src/main.cpp
  src/engine.cpp
  src/material.cpp
  src/renderer.cpp
)
target_include_directories(FallingSand PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenGL REQUIRED)
target_link_libraries(FallingSand PRIVATE
  glfw
  glad_gl_core_33
  OpenGL::GL
)

# === (Opcional) Rutas de shaders/assets ===
set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
add_definitions(-DSHADERS_DIR="${SHADERS_DIR}")
