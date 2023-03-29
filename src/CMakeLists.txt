# Common code
add_subdirectory(com)

# Shaders
add_subdirectory(shaders)

# Vulkan code
add_subdirectory(vk)

add_executable(${PROJECT_NAME}_vk)

target_compile_definitions(${PROJECT_NAME}_vk PRIVATE vulkan)

target_sources(${PROJECT_NAME}_vk PRIVATE main.cpp)

target_link_libraries(${PROJECT_NAME}_vk display::com display::vk glfw spdlog::spdlog)