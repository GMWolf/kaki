add_executable(image_packager image_packager.cpp)
target_link_libraries(image_packager PRIVATE kaki-asset-interface cereal kaki-common)

function(add_image imageName input)

    add_custom_command(
            OUTPUT ${imageName}.json ${imageName}.bin
            COMMAND image_packager ARGS ${CMAKE_CURRENT_LIST_DIR}/${input} ${imageName}.json ${imageName}
            DEPENDS "${input}" image_packager
            USES_TERMINAL
    )

    add_custom_target(${imageName} DEPENDS ${imageName}.json)
endfunction()