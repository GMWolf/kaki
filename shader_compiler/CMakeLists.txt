add_executable(shader_compiler shader_compiler.cpp)
target_link_libraries(shader_compiler PRIVATE shaderc)

function(compile_shader targetName)
    set(flags)
    set(args)
    set(listArgs SOURCES)
    cmake_parse_arguments(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

    foreach(source IN ITEMS ${arg_SOURCES})
    add_custom_command(
            OUTPUT ${source}.spv
            COMMAND shader_compiler ARGS ${CMAKE_CURRENT_LIST_DIR}/${source} ${source}.spv
            DEPENDS "${source}" shader_compiler
            DEPFILE ${source}.d
            USES_TERMINAL
    )
    list(APPEND outputFiles ${source}.spv)
    endforeach()

    add_custom_target(${targetName} DEPENDS ${outputFiles})
endfunction()