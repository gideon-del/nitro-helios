add_library(imgui-core STATIC
    imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp)

target_include_directories(imgui-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(imgui-backend-glfw STATIC backends/imgui_impl_glfw.cpp)
target_link_libraries(imgui-backend-glfw 
PUBLIC imgui-core 
glfw
)


add_library(imgui-backend-vulkan STATIC backends/imgui_impl_vulkan.cpp)
target_link_libraries(imgui-backend-vulkan PUBLIC imgui-core Vulkan::Vulkan)


if(APPLE)
    add_library(imgui-backend-metal STATIC backends/imgui_impl_metal.mm)
    target_link_libraries(imgui-backend-metal PUBLIC imgui-core)
endif()