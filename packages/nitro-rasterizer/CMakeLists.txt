cmake_minimum_required( VERSION 3.20)
project(nitro-rasterizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(nitro-rasterizer
src/rasterizer.cpp
src/pipeline.cpp
  src/main.cpp
)

target_link_libraries(nitro-rasterizer
    PRIVATE nitro-math
)

target_include_directories(nitro-rasterizer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)