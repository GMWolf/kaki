add_executable(packager packager.cpp)

target_link_libraries(packager PRIVATE kaki-asset-interface cereal kaki-common)


function(make_package targetName)
    set(flags)
    set(args)
    set(listArgs SOURCES)
    cmake_parse_arguments(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

    add_custom_command(
            OUTPUT ${targetName}.json ${targetName}.bin
            COMMAND packager ARGS ${targetName} ${arg_SOURCES}
            DEPENDS ${arg_SOURCES} packager
            USES_TERMINAL
    )

    add_custom_target(${targetName} DEPENDS ${targetName}.json)
endfunction()