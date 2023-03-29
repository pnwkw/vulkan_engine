add_library(com INTERFACE)
add_library(display::com ALIAS com)

target_include_directories(com INTERFACE include)
target_sources(com INTERFACE
        include/app_com.h
        include/isdebug.h
        include/glm_helper.h
)

target_link_libraries(com INTERFACE glfw glm::glm spdlog::spdlog)