cmake_minimum_required(VERSION 3.20)
project(nitro-renderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nitro-renderer STATIC
 src/mesh-renderer.cpp
 src/frame-resource.cpp
 src/passes/shadow-pass.cpp
 src/passes/geometry-pass.cpp
 src/panels.cpp
 src/passes/forward-lighting-pass.cpp
 src/passes/cascade-shadow-map-pass.cpp
 src/passes/deferred-lighting-pass.cpp
 src/renderers/forward-renderer.cpp
 src/renderers/deferred-renderer.cpp
 src/passes/depth-prepass.cpp
 src/passes/lighting-stencil-pass.cpp
 src/passes/tile-deffered-compute-pass.cpp
 src/passes/tiled-light-shading-pass.cpp
 src/renderers/tiled-deferred-renderer.cpp
 src/passes/debug-draw-pass.cpp
 vendor/tinygltf_impl.cpp
 src/scene.cpp
 vendor/stb_impl.cpp
 src/passes/tone-map-pass.cpp
 src/passes/main-scene-pass.cpp
 src/utils.cpp
 src/passes/skybox-pass.cpp
 src/passes/brightness-pass.cpp
 src/passes/gaussian-blur-pass.cpp
 src/passes/combine-texture-pass.cpp
 src/post-process/bloom-effect.cpp
 src/passes/auto-exposure-pass.cpp
 src/passes/color-grading-pass.cpp
 src/passes/ssao-pass.cpp
 src/passes/fxaa-pass.cpp
 src/render-graph.cpp
 src/passes/particle-update-pass.cpp
 src/passes/particle-billboard-pass.cpp
 src/passes/particle-emitter-pass.cpp
 src/passes/particle-compact-pass.cpp
 src/passes/particle-indirect-pass.cpp
 src/mesh-manager.cpp
 src/material-manager.cpp
 src/passes/mesh-compact-pass.cpp
)

find_package(glm REQUIRED)

target_include_directories(nitro-renderer
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_link_libraries(nitro-renderer 
PUBLIC nitro-rhi
PUBLIC nitro-geometry
PUBLIC glm::glm
PRIVATE imgui-core
PRIVATE imnodes
)