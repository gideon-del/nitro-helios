cmake_minimum_required(VERSION 3.20)
project(nitro-geometry VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nitro-geometry INTERFACE)
find_package(glm REQUIRED)


target_link_libraries(nitro-geometry 
INTERFACE nitro-rhi
INTERFACE glm::glm
)
target_include_directories(nitro-geometry
INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)