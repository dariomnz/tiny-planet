cmake_minimum_required(VERSION 3.16)
project(Tiny-planet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
FetchContent_MakeAvailable(glm)

add_executable(tiny-planet src/main.cpp src/camera.cpp)

target_include_directories(tiny-planet PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(tiny-planet PRIVATE
    glm::glm
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
    -sDEMANGLE_SUPPORT=1
    --shell-file=${CMAKE_SOURCE_DIR}/shell.html
)
