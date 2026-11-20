cmake_minimum_required(VERSION 3.20)
project(nitro-rhi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE NITRO_RHI_SOURCE CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(nitro-rhi STATIC ${NITRO_RHI_SOURCE})
find_package(glm REQUIRED)


target_include_directories(nitro-rhi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nitro-rhi PUBLIC glm::glm imgui-core nitro-core)

if(NITRO_BUILD_TESTS)

file(GLOB_RECURSE NITRO_RHI_TESTS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp)

        add_library(nitro-rhi-test-support INTERFACE)
target_include_directories(nitro-rhi-test-support INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/supports)      
target_link_libraries(nitro-rhi-test-support INTERFACE nitro-rhi)

add_executable(nitro-rhi-tests ${NITRO_RHI_TESTS})
target_link_libraries(nitro-rhi-tests PRIVATE nitro-rhi Catch2::Catch2WithMain nitro-rhi-test-support) 

include(Catch)
catch_discover_tests(nitro-rhi-tests)       

endif()