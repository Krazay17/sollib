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

set(NRI_ENABLE_D3D12_SUPPORT OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_D3D11_SUPPORT OFF CACHE BOOL "" FORCE)
set(SOL_LIBS "C:/libs")
option(RENDER_API_WGPU "Link to webgpu" ON)
option(RENDER_API_NRI "Link to webgpu" OFF)
set(RENDER_LIB webgpu)

add_compile_options(
    -Wno-missing-field-initializers 
    -Wno-unused-parameter
    -Wno-nullability-completeness
    -Wno-error
)

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

if(RENDER_API_WGPU)
    list(FILTER SOURCES EXCLUDE REGEX "gfx_nri.cpp")
elseif(RENDER_API_NRI)
    list(FILTER SOURCES EXCLUDE REGEX "gfx_wgpu.cpp")

    set(NRI_LIB_DIR "${SOL_LIBS}/NRI/build/Release")
    add_library(NRI STATIC IMPORTED)
    set_target_properties(NRI PROPERTIES
        IMPORTED_LOCATION "C:/libs/NRI/_Bin/Release/NRI.lib"
    )
    foreach(lib NRI_Shared NRI_VK NRI_Validation)
        add_library(${lib} STATIC IMPORTED)
        set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION "${NRI_LIB_DIR}/${lib}.lib")
    endforeach()

    # one umbrella target, also fixes link order
    add_library(NRI_All INTERFACE)
    target_link_libraries(NRI_All INTERFACE NRI NRI_VK NRI_Validation NRI_Shared)
    target_include_directories(NRI_All INTERFACE "${SOL_LIBS}/NRI/Include")
    target_compile_definitions(NRI_All INTERFACE NRI_STATIC_LIBRARY=1)

    set(RENDER_LIB NRI_All)
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
target_precompile_headers(sollib PRIVATE ${CMAKE_SOURCE_DIR}/src/core/pch.hpp)
target_include_directories(sollib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${SOL_LIBS}/NRI/Include
    ${SOL_LIBS}/glfw/include
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
    add_library(webgpu STATIC IMPORTED)
    set_target_properties(webgpu PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SOL_LIBS}/include"
        IMPORTED_LOCATION "${SOL_LIBS}/lib/wgpu_native.lib"
    )
    add_library(Jolt::Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt::Jolt PROPERTIES
        IMPORTED_LOCATION "${SOL_LIBS}/JoltPhysics/out/windows-release/Jolt.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SOL_LIBS}/JoltPhysics"
    )
    if(WIN32)
    target_link_libraries(sollib PRIVATE
        ws2_32 bcrypt userenv ntdll advapi32 kernel32 user32 gdi32 shell32
        glfw
        ${RENDER_LIB}
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