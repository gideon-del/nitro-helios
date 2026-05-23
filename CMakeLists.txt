cmake_minimum_required(VERSION 3.20)
project(NitroHelios VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Vulkan REQUIRED)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders")
file(GLOB SHADER_SOURCES "shaders/*.vert" "shaders/*.frag")

foreach(SOURCE ${SHADER_SOURCES})
    get_filename_component(FILE_NAME ${SOURCE} NAME)
    set(OUT_FILE "${CMAKE_BINARY_DIR}/bin/shaders/${FILE_NAME}.spv")
    add_custom_command(OUTPUT ${OUT_FILE} COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SOURCE} -o ${OUT_FILE} DEPENDS ${SOURCE})
    list(APPEND SPIRV_BINARIES ${OUT_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_BINARIES})

if(APPLE) 
   
    find_program(XCRUN xcrun)
    file(GLOB METAL_SOURCES "shaders/*.metal")
    foreach(SOURCE ${METAL_SOURCES})
        get_filename_component(FILE_NAME ${SOURCE} NAME_WE)
        set(AIR_FILE "${CMAKE_BINARY_DIR}/bin/shaders/${FILE_NAME}.air")
        set(LIB_FILE "${CMAKE_BINARY_DIR}/bin/shaders/${FILE_NAME}.metallib")
           add_custom_command(
            OUTPUT ${LIB_FILE}
            COMMAND ${XCRUN} -sdk macosx metal -c ${SOURCE} -o ${AIR_FILE}
            COMMAND ${XCRUN} -sdk macosx metallib ${AIR_FILE} -o ${LIB_FILE}
            DEPENDS ${SOURCE}
        )
        list(APPEND METALLIB_BINARIES ${LIB_FILE})
    endforeach()
    add_custom_target(CompileMetalShaders ALL DEPENDS ${METALLIB_BINARIES})    
endif()
add_subdirectory(packages/nitro-math)
add_subdirectory(packages/nitro-rasterizer)
add_subdirectory(packages/nitro-rhi)
add_subdirectory(packages/nitro-rhi-backends)
add_subdirectory(packages/nitro-rhi-test)