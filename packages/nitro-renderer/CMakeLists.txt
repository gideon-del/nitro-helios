cmake_minimum_required(VERSION 3.20)
project(nitro-renderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE NITRO_RENDERER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(nitro-renderer STATIC ${NITRO_RENDERER_SOURCES}
)

find_package(glm REQUIRED)

target_include_directories(nitro-renderer
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(NITRO_BUILD_TESTS)

file(GLOB_RECURSE NITRO_RENDERER_TESTS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp)

  add_executable(nitro-renderer-tests ${NITRO_RENDERER_TESTS})
  
  target_link_libraries(nitro-renderer-tests PRIVATE nitro-renderer Catch2::Catch2WithMain)
  target_include_directories(nitro-renderer-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  include(Catch)
  catch_discover_tests(nitro-renderer-tests)
endif()

target_link_libraries(nitro-renderer 
PUBLIC nitro-assets
PUBLIC nitro-rhi
PUBLIC nitro-geometry
PUBLIC glm::glm
PUBLIC nitro-core
PRIVATE imgui-core
PRIVATE imnodes
PRIVATE meshoptimizer
PRIVATE imguizmo
PRIVATE stb_image
PRIVATE tiny_gltf
PRIVATE nlohmann_json
PRIVATE nfd
)