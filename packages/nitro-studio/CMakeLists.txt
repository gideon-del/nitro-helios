cmake_minimum_required(VERSION 3.20)
project(nitro-studio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE NITRO_STUDIO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(nitro-studio  ${NITRO_STUDIO_SOURCES})



target_include_directories(nitro-studio
PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(glfw3 REQUIRED)

target_compile_definitions(nitro-studio  PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/bin/shaders"
)

target_link_libraries(nitro-studio PRIVATE
    nitro-rhi-backends
    nitro-geometry
    nitro-renderer
    glfw
    imgui-core
    imguizmo
)
if(APPLE)
    target_link_libraries(nitro-studio PRIVATE nitro-rhi-metal)
endif()