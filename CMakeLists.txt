cmake_minimum_required(VERSION 3.16)
project(Tiny-planet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This project is web-only: build with emcmake (Emscripten).")
endif()

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b-docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm imgui)

# Dear ImGui (docking branch) + GLFW + OpenGL3 backends for Emscripten/WebGL2.
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)

file(GLOB_RECURSE TINY_PLANET_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(tiny-planet ${TINY_PLANET_SOURCES})

target_include_directories(tiny-planet PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(tiny-planet PRIVATE
    glm::glm
    imgui
)

# Output .html (Emscripten generates .html + .js + .wasm)
set_target_properties(tiny-planet PROPERTIES SUFFIX ".html")

# Emscripten GLFW port + WebGL2 (OpenGL ES 3.0)
target_compile_options(tiny-planet PRIVATE
    -fexceptions
)
target_link_options(tiny-planet PRIVATE
    -sUSE_GLFW=3
    -sFULL_ES3=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sALLOW_MEMORY_GROWTH=1
    -sMINIFY_HTML=0
    -fexceptions
    -sDISABLE_EXCEPTION_CATCHING=0
    -sASSERTIONS=1
    --shell-file=${CMAKE_SOURCE_DIR}/shell.html
)
