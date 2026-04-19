cmake_minimum_required(VERSION 3.20)
project(nitro-math VERSION 1.0.0 LANGUAGES CXX)


# Headers
add_library(nitro-math INTERFACE)
target_include_directories(
    nitro-math
    INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR NITROHELIOS_BUILD_TESTS)

include(FetchContent)

 FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
  )
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  enable_testing()
  add_executable(nitro-math-tests
    tests/test_vec.cpp  
    tests/test_mat.cpp 
    tests/test_quat.cpp 
    tests/test_utils.cpp 

  )
  target_link_libraries(nitro-math-tests
    PRIVATE nitro-math Catch2::Catch2WithMain
  )
  include(Catch)
  set_target_properties(nitro-math-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
  catch_discover_tests(nitro-math-tests
   TEST_PREFIX "nitro-math::"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DISCOVERY_MODE PRE_TEST
  )
endif()