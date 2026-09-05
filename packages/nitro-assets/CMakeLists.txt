cmake_minimum_required(VERSION 3.20)
project(nitro-assets VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

add_library(nitro-assets STATIC 
src/registry.cpp
)
find_package(glm REQUIRED)
target_include_directories(
   nitro-assets PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_link_libraries(nitro-assets
PUBLIC nitro-core
PRIVATE stb_image
PUBLIC nitro-geometry
PUBLIC glm::glm
PUBLIC nlohmann_json
)