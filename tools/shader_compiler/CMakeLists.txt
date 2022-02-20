add_executable(shader_compiler shader_compiler.cpp)
target_link_libraries(shader_compiler PRIVATE
        shaderc
        spirv-reflect
        vulkan
        cereal
        kaki-gfx-interface
        kaki-asset-interface
        kaki-common)

function(compile_shader targetName)
    set(flags)
    set(args)
    set(listArgs SOURCES)
    cmake_parse_arguments(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

    foreach(source IN ITEMS ${arg_SOURCES})
    add_custom_command(
            OUTPUT ${source}.shd
            COMMAND shader_compiler ARGS ${CMAKE_CURRENT_LIST_DIR}/${source} ${source}.shd
            DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${source}" shader_compiler
            #DEPFILE ${source}.d
            USES_TERMINAL
    )
    list(APPEND outputFiles ${source}.shd)
    endforeach()

    add_custom_target(${targetName} DEPENDS ${outputFiles})
endfunction()