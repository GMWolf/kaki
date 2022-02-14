add_executable(gltf_compiler gltf_compiler.cpp)
target_link_libraries(gltf_compiler PRIVATE cgltf cereal glm kaki-asset-interface kaki-common)

function(compile_gltf targetName)
    set(flags)
    set(args)
    set(listArgs SOURCES)
    cmake_parse_arguments(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

    foreach(source IN ITEMS ${arg_SOURCES})
        add_custom_command(
                OUTPUT ${source}.bin
                COMMAND gltf_compiler ARGS ${CMAKE_CURRENT_LIST_DIR}/${source} ${source}.bin
                DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${source}" gltf_compiler
                #DEPFILE ${source}.d
                USES_TERMINAL
        )
        list(APPEND outputFiles ${source}.bin)
    endforeach()

    add_custom_target(${targetName} DEPENDS ${outputFiles})
endfunction()