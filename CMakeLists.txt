cmake_minimum_required(VERSION 3.25)

project(AI3 VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

option(AI3_TERMUX "Build for the Termux Unix userspace" OFF)
option(AI3_BUILD_TESTS "Build AI3 tests" ON)
option(AI3_HEADLESS "Build only display-independent AI3 targets" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5
    GIT_SHALLOW FALSE
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed
    GIT_SHALLOW FALSE
)

FetchContent_MakeAvailable(glm)

add_library(ai3_options STATIC src/app/command_line.cpp)
target_include_directories(ai3_options PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(ai3_editor STATIC src/editor/editor_state.cpp)
target_include_directories(ai3_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ai3_editor PUBLIC glm::glm)

add_library(ai3_scene STATIC
    src/scene/length_units.cpp
    src/scene/orbit_camera.cpp
    src/scene/render_target_size.cpp
    src/scene/scene_math.cpp
    src/scene/sphere_mesh.cpp
)
target_include_directories(ai3_scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ai3_scene PUBLIC ai3_editor)

add_library(ai3_localization STATIC
    src/localization/localization.cpp
    src/localization/resource_locator.cpp
)
target_include_directories(ai3_localization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(ai3_localization PRIVATE
    AI3_SOURCE_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

add_custom_target(ai3_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "Copying AI3 runtime resources"
)

if(NOT AI3_HEADLESS)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
    set(SDL_OPENGLES ON CACHE BOOL "" FORCE)
    set(SDL_VULKAN OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03
        GIT_SHALLOW FALSE
    )

    # This immutable revision is from Dear ImGui's docking branch.
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG fd13a1e8923a0a7077b404fc36fd063b25a0c0b5
        GIT_SHALLOW FALSE
    )

    FetchContent_MakeAvailable(SDL3 imgui)

    find_library(AI3_GLES_LIBRARY NAMES GLESv3 GLESv2 REQUIRED)
    if(AI3_TERMUX)
        find_library(AI3_ANDROID_SHMEM_LIBRARY NAMES android-shmem REQUIRED)
    endif()

    add_library(ai3_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_compile_definitions(ai3_imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
    target_include_directories(ai3_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(ai3_imgui PUBLIC SDL3::SDL3 ${AI3_GLES_LIBRARY})

    add_executable(ai3
        src/main.cpp
        src/app/application.cpp
        src/platform/sdl_gl_platform.cpp
        src/render/viewport_renderer.cpp
        src/ui/editor_ui.cpp
        src/ui/imgui_layer.cpp
    )
    target_include_directories(ai3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(ai3 PRIVATE ai3_imgui ${AI3_ANDROID_SHMEM_LIBRARY})
    target_link_libraries(ai3 PRIVATE ai3_options ai3_editor ai3_scene)
    target_link_libraries(ai3 PRIVATE ai3_localization)
    add_dependencies(ai3 ai3_resources)
endif()

function(ai3_enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if(TARGET ai3)
    ai3_enable_warnings(ai3)
endif()
ai3_enable_warnings(ai3_options)
ai3_enable_warnings(ai3_editor)
ai3_enable_warnings(ai3_scene)
ai3_enable_warnings(ai3_localization)

if(AI3_BUILD_TESTS)
    FetchContent_MakeAvailable(doctest)
    enable_testing()
    add_executable(ai3_tests
        tests/command_line_test.cpp
        tests/editor_state_test.cpp
        tests/localization_test.cpp
        tests/scene_test.cpp
        tests/ui_identity_test.cpp
    )
    target_link_libraries(ai3_tests PRIVATE ai3_options ai3_editor ai3_scene ai3_localization doctest::doctest)
    ai3_enable_warnings(ai3_tests)
    add_test(NAME ai3_unit_tests COMMAND ai3_tests)
    add_test(NAME ai3_resource_copy_test
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_ASSETS=${CMAKE_CURRENT_SOURCE_DIR}/assets
            -DBUILT_ASSETS=${CMAKE_CURRENT_BINARY_DIR}/assets
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/resource_copy_test.cmake
    )
    if(NOT AI3_HEADLESS)
        add_test(NAME ai3_smoke_test COMMAND ai3 --smoke-test)
        set_tests_properties(ai3_smoke_test PROPERTIES LABELS display TIMEOUT 30)
    endif()
endif()

if(AI3_HEADLESS)
    set(ai3_headless_targets ai3_options ai3_editor ai3_scene ai3_localization)
    if(TARGET ai3_tests)
        list(APPEND ai3_headless_targets ai3_tests)
    endif()
    foreach(target IN LISTS ai3_headless_targets)
        get_target_property(link_libraries ${target} LINK_LIBRARIES)
        get_target_property(interface_link_libraries ${target} INTERFACE_LINK_LIBRARIES)
        set(all_link_libraries ${link_libraries} ${interface_link_libraries})
        foreach(link_library IN LISTS all_link_libraries)
            if(link_library MATCHES "SDL|imgui|GLES|EGL|OpenGL")
                message(FATAL_ERROR "Headless target ${target} links graphical dependency ${link_library}")
            endif()
        endforeach()
    endforeach()
endif()
