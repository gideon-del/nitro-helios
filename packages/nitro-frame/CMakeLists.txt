cmake_minimum_required(VERSION 3.20)
project(nitro-renderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE NITRO_FRAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(nitro-frame STATIC ${NITRO_FRAME_SOURCES})



target_include_directories(nitro-frame
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)


if(NITRO_BUILD_TESTS)
file(GLOB_RECURSE NITRO_FRAME_TESTS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp)

  add_executable(nitro-frame-tests ${NITRO_FRAME_TESTS})
  
  target_link_libraries(nitro-frame-tests PRIVATE nitro-frame Catch2::Catch2WithMain)
  target_include_directories(nitro-frame-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  include(Catch)
  catch_discover_tests(nitro-frame-tests)
endif()


target_link_libraries(nitro-frame 
PUBLIC nitro-rhi
PUBLIC nitro-core
)