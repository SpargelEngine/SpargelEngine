if (SPARGEL_IS_MACOS)
    spargel_add_executable(
        NAME demo_metal
        PRIVATE_MACOS
            demo_metal.mm
        DEPS
            base
            resource
            ui
    )
    # spargel_application_add_resources(
    #     TARGET demo_metal
    #     SRC ${CMAKE_CURRENT_SOURCE_DIR}/demo_assets
    #     DST
    #     DEPS
    # )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.ir
        COMMAND
            xcrun metal -frecord-sources -o ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.ir -c ${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.metal
        DEPENDS shader/shader.metal
    )
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib
        COMMAND
            xcrun metallib -o ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.ir
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.ir
    )
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shader/metal_shader.generated.h
        COMMAND
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/gen.py
                --data ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib
                --output-dir ${CMAKE_CURRENT_BINARY_DIR}/shader/
                --file-name metal_shader
                --var-name spargel_render_metal_shader
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/gen.py
    )
    # add_custom_command(
    #     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/shader.metallib
    #     COMMAND
    #         cp ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib ${CMAKE_CURRENT_BINARY_DIR}/resources/shader.metallib
    #     DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib
    # )
    add_custom_target(demo_render_metal_shaders
        DEPENDS
            # ${CMAKE_CURRENT_BINARY_DIR}/shader/shader.metallib
            ${CMAKE_CURRENT_BINARY_DIR}/shader/metal_shader.generated.h
    )
    add_dependencies(demo_metal demo_render_metal_shaders)
endif ()


if (SPARGEL_ENABLE_OPENGL)

    spargel_add_executable(
        NAME demo_opengl
        PRIVATE
            demo_opengl.cpp
        DEPS
            glad
            resource
            ui
    )

    if (SPARGEL_IS_WINDOWS)
        find_package(OpenGL REQUIRED)
        target_link_libraries(demo_opengl PRIVATE OpenGL::GL)
    endif ()

    add_custom_target(demo_opengl_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/demo_opengl.vs ${CMAKE_CURRENT_BINARY_DIR}/resources/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/demo_opengl.fs ${CMAKE_CURRENT_BINARY_DIR}/resources/
    )

    add_dependencies(demo_opengl demo_opengl_shaders)

endif ()
