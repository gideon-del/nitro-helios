cmake_minimum_required(VERSION 3.20)
project(nitro-renderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nitro-renderer STATIC
 src/mesh-renderer.cpp
 src/frame-resource.cpp
 src/render-object.cpp
 src/passes/shadow-pass.cpp
 src/passes/geometry-pass.cpp
)

find_package(glm REQUIRED)

target_include_directories(nitro-renderer
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(nitro-renderer 
PUBLIC nitro-rhi
PUBLIC nitro-geometry
PUBLIC glm::glm
)