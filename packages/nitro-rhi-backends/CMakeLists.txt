cmake_minimum_required(VERSION 3.20)
project(nitro-rhi-backends VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3  REQUIRED)

add_library(nitro-rhi-backends STATIC
src/vulkan/vulkan-device.cpp
src/vulkan/vulkan-buffer.cpp
src/vulkan/vulkan-texture.cpp
src/vulkan/vulkan-surface.cpp
src/vendor/vma_impl.cpp
src/vulkan/vulkan-pipeline.cpp
src/vulkan/vulkan-swapchain.cpp
)


target_include_directories(nitro-rhi-backends
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_link_libraries(nitro-rhi-backends
PUBLIC nitro-rhi
PUBLIC Vulkan::Vulkan
PUBLIC glfw
)