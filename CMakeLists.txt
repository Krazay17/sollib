cmake_minimum_required(VERSION 4.2.1)
project(Sol_Library C CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(SOL_LIBS "C:/libs")

# Dependencies
add_subdirectory(
    ${SOL_LIBS}/glm
    ${CMAKE_BINARY_DIR}/glm
    EXCLUDE_FROM_ALL
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
if(EMSCRIPTEN)
    list(FILTER SOURCES EXCLUDE REGEX "win_net\\.cpp$")
else()
    list(FILTER SOURCES EXCLUDE REGEX "web_net\\.cpp$")
endif()

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/src/*.hpp
)

# Library
add_library(sollib
    ${SOURCES}
)
target_sources(sollib PRIVATE
    ${HEADERS}
)
target_include_directories(sollib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)
if(EMSCRIPTEN)
    add_library(Jolt::Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt::Jolt PROPERTIES
        IMPORTED_LOCATION
            "${SOL_LIBS}/JoltPhysics/out/wasm-release/libJolt.a"
        INTERFACE_INCLUDE_DIRECTORIES
            "${SOL_LIBS}/JoltPhysics"
    )
    set(SOL_EM_PORTS --use-port=emdawnwebgpu --use-port=contrib.glfw3)
    target_compile_options(sollib PRIVATE ${SOL_EM_PORTS})
else()
    find_package(glfw3 REQUIRED)
    add_library(webgpu SHARED IMPORTED)
    set_target_properties(webgpu PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SOL_LIBS}/include"
        IMPORTED_IMPLIB "${SOL_LIBS}/lib/wgpu_native.lib"
    )
    add_library(Jolt::Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt::Jolt PROPERTIES
        IMPORTED_LOCATION "${SOL_LIBS}/JoltPhysics/out/windows-release/Jolt.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SOL_LIBS}/JoltPhysics"
    )
    target_link_libraries(sollib PUBLIC glfw webgpu)
    if(WIN32)
    target_link_libraries(sollib PRIVATE
        ws2_32 bcrypt userenv ntdll advapi32 kernel32 user32 gdi32 shell32
    )
    endif()
endif()
target_link_libraries(sollib PUBLIC Jolt::Jolt glm::glm)

# Executable
add_executable(solapp)
target_link_libraries(solapp PRIVATE sollib)
if(EMSCRIPTEN)
    target_sources(solapp PRIVATE 
        ${CMAKE_SOURCE_DIR}/platform/web/web_main.cpp
        ${CMAKE_SOURCE_DIR}/game/game.cpp
    )
    target_link_options(solapp PRIVATE
        ${SOL_EM_PORTS}
        --shell-file ${CMAKE_SOURCE_DIR}/platform/web/shell.html
        "-sGROWABLE_ARRAYBUFFERS=2"
        -lwebsocket.js
    )
    set_target_properties(solapp PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
    )
else()
    target_sources(solapp PRIVATE
        ${CMAKE_SOURCE_DIR}/platform/win/win_main.cpp
        ${CMAKE_SOURCE_DIR}/game/game.cpp
    )
    set_target_properties(solapp PROPERTIES OUTPUT_NAME "SolApp")
endif()