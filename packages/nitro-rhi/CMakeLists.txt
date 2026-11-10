cmake_minimum_required(VERSION 3.20)
project(nitro-rhi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(nitro-rhi INTERFACE)
find_package(glm REQUIRED)


target_include_directories(nitro-rhi
INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(nitro-rhi INTERFACE glm::glm imgui-core nitro-core)

if(NITRO_BUILD_TESTS)

file(GLOB_RECURSE NITRO_RHI_TESTS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp)

add_executable(nitro-rhi-tests ${NITRO_RHI_TESTS})
target_link_libraries(nitro-rhi-tests PRIVATE nitro-rhi Catch2::Catch2WithMain) 
target_include_directories(nitro-rhi-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
include(Catch)
catch_discover_tests(nitro-rhi-tests)       

endif()