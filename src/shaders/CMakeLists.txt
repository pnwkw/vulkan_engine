function(add_spirv_target target_name infile)
    get_filename_component(outfile "${infile}" NAME_WE)
    add_custom_target(${target_name} DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${outfile}.spv")
    add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${outfile}.spv"
            COMMAND ${Vulkan_INCLUDE_DIR}/../Bin/glslc -O ${CMAKE_CURRENT_SOURCE_DIR}/${infile} -o ${CMAKE_CURRENT_BINARY_DIR}/${outfile}.spv
            COMMAND ${Vulkan_INCLUDE_DIR}/../Bin/spirv-opt ${CMAKE_CURRENT_BINARY_DIR}/${outfile}.spv --print-all -O -o ${CMAKE_CURRENT_BINARY_DIR}/${outfile}.spv 2> ${CMAKE_CURRENT_BINARY_DIR}/${outfile}_optimized.spvasm
            COMMAND ${Vulkan_INCLUDE_DIR}/../Bin/glslc -O -S ${CMAKE_CURRENT_SOURCE_DIR}/${infile} -o ${CMAKE_CURRENT_BINARY_DIR}/${outfile}.spvasm
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${infile}"
    )
endfunction()

add_spirv_target(passthrough passthrough.vert)
add_spirv_target(red red.frag)

add_library(triangle_shader INTERFACE)
add_dependencies(triangle_shader passthrough red)
add_library(display::program::triangle_shader ALIAS triangle_shader)
