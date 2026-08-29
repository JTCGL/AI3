cmake_minimum_required(VERSION 3.25)

project(AI3 VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

option(AI3_TERMUX "Build for the Termux Unix userspace" OFF)
option(AI3_BUILD_TESTS "Build AI3 tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5
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

add_library(ai3_options STATIC src/app/command_line.cpp)
target_include_directories(ai3_options PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(ai3
    src/main.cpp
    src/app/application.cpp
    src/platform/sdl_gl_platform.cpp
    src/ui/editor_ui.cpp
    src/ui/imgui_layer.cpp
)
target_include_directories(ai3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ai3 PRIVATE ai3_imgui ${AI3_ANDROID_SHMEM_LIBRARY})
target_link_libraries(ai3 PRIVATE ai3_options)

function(ai3_enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

ai3_enable_warnings(ai3)
ai3_enable_warnings(ai3_options)

if(AI3_BUILD_TESTS)
    FetchContent_MakeAvailable(doctest)
    enable_testing()
    add_executable(ai3_tests tests/command_line_test.cpp)
    target_link_libraries(ai3_tests PRIVATE ai3_options doctest::doctest)
    ai3_enable_warnings(ai3_tests)
    add_test(NAME ai3_unit_tests COMMAND ai3_tests)
    add_test(NAME ai3_smoke_test COMMAND ai3 --smoke-test)
    set_tests_properties(ai3_smoke_test PROPERTIES LABELS display TIMEOUT 30)
endif()
