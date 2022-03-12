add_executable(pipeline_compiler pipeline_compiler.cpp shader.h)
target_link_libraries(pipeline_compiler PRIVATE
        kaki-asset-interface
        cereal
        kaki-common
        rapidjson
        vk_mini_libs
        kaki-gfx-interface
        shaderc
        spirv-reflect)

function(compile_pipeline targetName input)

    add_custom_command(
            OUTPUT ${targetName}.json ${targetName}.bin
            COMMAND pipeline_compiler ARGS ${CMAKE_CURRENT_LIST_DIR}/${input} ${targetName}.json
            DEPENDS "${input}" pipeline_compiler
            USES_TERMINAL
    )

    add_custom_target(${targetName} DEPENDS ${targetName}.json)
endfunction()