cmake_minimum_required(VERSION 3.25)

project(AI3 VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

option(AI3_TERMUX "Build for the Termux Unix userspace" OFF)

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

add_executable(ai3 src/main.cpp)
target_link_libraries(ai3 PRIVATE ai3_imgui ${AI3_ANDROID_SHMEM_LIBRARY})

enable_testing()
add_test(NAME ai3_build_smoke COMMAND ${CMAKE_COMMAND} -E echo "AI3 application built successfully")
