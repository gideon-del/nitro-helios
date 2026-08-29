cmake_minimum_required(VERSION 3.20)
project(nitro-core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nitro-core INTERFACE)

target_compile_features(nitro-core INTERFACE cxx_std_23)
target_include_directories(
   nitro-core INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/includes
)