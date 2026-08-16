file(
    GLOB MESHOPTIMIZER_SOURCES
    CONFIGURE_DEPENDS
    "*.cpp"
)

add_library(meshoptimizer STATIC 
${MESHOPTIMIZER_SOURCES}
)

target_include_directories(meshoptimizer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)