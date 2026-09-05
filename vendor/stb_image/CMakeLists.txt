
add_library(
    stb_image STATIC
     stb_impl.cpp
)

target_link_libraries(
    stb_image
    PUBLIC nlohmann_json
)

target_include_directories(
  stb_image 
  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR} 
)