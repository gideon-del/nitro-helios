cmake_minimum_required(VERSION 3.20)
project(nitro-rhi-backends VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm REQUIRED)
add_library(nitro-rhi-backends STATIC
src/vulkan/vulkan-device.cpp
src/vulkan/vulkan-buffer.cpp
src/vulkan/vulkan-texture.cpp
src/vulkan/vulkan-surface.cpp
src/vendor/vma_impl.cpp
src/vulkan/vulkan-pipeline.cpp
src/vulkan/vulkan-swapchain.cpp
src/vulkan/vulkan-command-buffer.cpp
)



target_include_directories(nitro-rhi-backends
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

if(APPLE) 
 add_library(nitro-rhi-metal STATIC
        src/metal/metal_cpp_impl.cpp
        src/metal/metal-device.cpp
        src/metal/metal-swapchain.cpp
        src/metal/metal-pipeline.cpp
        src/metal/metal-command-buffer.cpp
        src/metal/metal-buffer.cpp
        src/metal/metal-cocoa-bridge.mm
    )

    target_include_directories(nitro-rhi-metal
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor/metal-cpp
    )

    target_link_libraries(nitro-rhi-metal
        PUBLIC nitro-rhi
        PUBLIC glfw
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
    )
endif()

target_link_libraries(nitro-rhi-backends
PUBLIC nitro-rhi
PUBLIC Vulkan::Vulkan
PUBLIC glfw
PUBLIC glm::glm
 $<$<PLATFORM_ID:Darwin>:nitro-rhi-metal>
)
