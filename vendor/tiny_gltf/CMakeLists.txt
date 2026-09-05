add_library(
    tiny_gltf STATIC 
     tinygltf_impl.cpp
)

target_link_libraries(
    tiny_gltf 
    PUBLIC stb_image
)

target_include_directories(
    tiny_gltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)