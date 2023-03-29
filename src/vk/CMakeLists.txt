add_library(vk STATIC)
add_library(display::vk ALIAS vk)

target_include_directories(vk PUBLIC include)
target_include_directories(vk PRIVATE src)

target_sources(vk PRIVATE
        src/app_vk.cpp
        src/engine_vk.cpp
        src/swapchain.cpp
        src/pipeline.cpp
        src/renderpass.cpp
        src/triangle_renderer.cpp
        src/vk_helper.h)

target_link_libraries(vk display::com display::program::triangle_shader glfw spdlog::spdlog Vulkan::Vulkan)
