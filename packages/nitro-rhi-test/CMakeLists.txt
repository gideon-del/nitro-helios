cmake_minimum_required(VERSION 3.20)
project(nitro-rhi-test)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)

add_executable(nitro-rhi-test 
 src/main.cpp
)

target_compile_definitions(nitro-rhi-test PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/bin/shaders"
)


if(APPLE)
    target_link_libraries(nitro-rhi-test
        PRIVATE nitro-rhi-backends
        PRIVATE nitro-rhi-metal
        PRIVATE glfw
    )
    # target_compile_definitions(nitro-rhi-test PRIVATE USE_METAL)
else()
    target_link_libraries(nitro-rhi-test
        PRIVATE nitro-rhi-backends
        PRIVATE glfw
    )
endif()