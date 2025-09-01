cmake_minimum_required(VERSION 3.16)
project(FallingSand VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === FetchContent para dependencias ===
include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLAD
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# === Ejecutable principal ===
add_executable(FallingSand
    src/main.cpp
    src/engine.cpp
    src/particle.cpp
)

# === Includes ===
target_include_directories(FallingSand PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${glad_SOURCE_DIR}/include
    ${glfw_SOURCE_DIR}/include
)

# === Link libraries ===
find_package(OpenGL REQUIRED)
target_link_libraries(FallingSand PRIVATE glfw glad OpenGL::GL)

# === Rutas de shaders ===
set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
add_definitions(-DSHADERS_DIR="${SHADERS_DIR}")
