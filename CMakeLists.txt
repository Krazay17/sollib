cmake_minimum_required(VERSION 4.2.1)
project(sollib CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(solapp
    "src/main.cpp"
    "src/render/render_wgpu_class.cpp"
    "src/render/wgsl_shaders.cpp"
    "include/sol/types.hpp"
)

if(EMSCRIPTEN)
    set(SOL_EM_PORTS --use-port=emdawnwebgpu --use-port=contrib.glfw3)
    target_compile_options(solapp PRIVATE ${SOL_EM_PORTS})
    target_link_options(solapp PRIVATE ${SOL_EM_PORTS} --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html)

    set_target_properties(solapp PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html")
else()
    find_package(glfw3 REQUIRED)
    add_library(webgpu SHARED IMPORTED)
    set_target_properties(webgpu PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "C:/libs/include"
        IMPORTED_IMPLIB "C:/libs/lib/wgpu_native.lib"
    )
    target_link_libraries(solapp PRIVATE glfw webgpu)

    if(WIN32)
        target_link_libraries(solapp PRIVATE
            ws2_32
            bcrypt
            userenv
            ntdll
            advapi32
            kernel32
            user32
            gdi32
            shell32
        )
    endif()

    set_target_properties(solapp PROPERTIES OUTPUT_NAME "SolApp")
endif()