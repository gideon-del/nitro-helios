cmake_minimum_required(VERSION 3.20)
project(NitroHelios VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Vulkan REQUIRED)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders")
file(GLOB_RECURSE SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
)

foreach(SOURCE ${SHADER_SOURCES})
file(RELATIVE_PATH REL_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "${SOURCE}"
    )
   
    set(OUT_FILE "${CMAKE_BINARY_DIR}/bin/shaders/${REL_PATH}.spv")
     get_filename_component(OUT_DIR ${OUT_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${OUT_DIR})
    add_custom_command(OUTPUT ${OUT_FILE} COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SOURCE} -o ${OUT_FILE} DEPENDS ${SOURCE})
    list(APPEND SPIRV_BINARIES ${OUT_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_BINARIES})

if(APPLE)

    find_program(XCRUN xcrun)

    file(GLOB_RECURSE METAL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.metal"
    )

    foreach(SOURCE ${METAL_SOURCES})

        # Relative path from shaders folder
        file(RELATIVE_PATH REL_PATH
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "${SOURCE}"
        )

        # Remove extension
        get_filename_component(REL_PATH_NO_EXT ${REL_PATH} NAME_WE)
        get_filename_component(REL_DIR ${REL_PATH} DIRECTORY)

        # Output paths
        set(AIR_FILE
            "${CMAKE_BINARY_DIR}/bin/shaders/${REL_DIR}/${REL_PATH_NO_EXT}.air"
        )

        set(LIB_FILE
            "${CMAKE_BINARY_DIR}/bin/shaders/${REL_DIR}/${REL_PATH_NO_EXT}.metallib"
        )

        # Create output directory
        get_filename_component(OUT_DIR ${LIB_FILE} DIRECTORY)
        file(MAKE_DIRECTORY ${OUT_DIR})

        add_custom_command(
            OUTPUT ${LIB_FILE}

            COMMAND ${XCRUN}
                -sdk macosx metal
                -frecord-sources
                -gline-tables-only
                -c ${SOURCE}
                -o ${AIR_FILE}

            COMMAND ${XCRUN}
                -sdk macosx metallib
                ${AIR_FILE}
                -o ${LIB_FILE}

            DEPENDS ${SOURCE}
        )

        list(APPEND METALLIB_BINARIES ${LIB_FILE})

    endforeach()

    add_custom_target(
        CompileMetalShaders ALL
        DEPENDS ${METALLIB_BINARIES}
    )

endif()
add_subdirectory(packages/nitro-math)
add_subdirectory(packages/nitro-rasterizer)
add_subdirectory(packages/nitro-rhi)
add_subdirectory(packages/nitro-rhi-backends)
add_subdirectory(packages/nitro-rhi-test)
add_subdirectory(packages/nitro-geometry)
add_subdirectory(packages/nitro-renderer)