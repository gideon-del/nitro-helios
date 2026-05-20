cmake_minimum_required(VERSION 3.20)
project(nitro-rhi-backends VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(nitro-rhi-backends STATIC
src/vulkan/vulkan-device.cpp
)


target_include_directories(nitro-rhi-backends
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(nitro-rhi-backends
PUBLIC nitro-rhi
)