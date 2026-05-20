cmake_minimum_required(VERSION 3.20)
project(nitro-rhi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(nitro-rhi INTERFACE)


target_include_directories(nitro-rhi
INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)