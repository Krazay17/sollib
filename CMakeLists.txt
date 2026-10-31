cmake_minimum_required(VERSION 4.2.1)
project(Sol_Library C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(
    C:/libs/glm
    ${CMAKE_BINARY_DIR}/glm
    EXCLUDE_FROM_ALL
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.hpp
)

add_library(sollib
    ${SOURCES}
)
target_sources(sollib PRIVATE
    ${HEADERS}
)
target_include_directories(sollib PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(solapp)
target_link_libraries(solapp PRIVATE sollib)
target_include_directories(solapp PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(EMSCRIPTEN)
    add_library(Jolt::Jolt STATIC IMPORTED GLOBAL)

    set_target_properties(Jolt::Jolt PROPERTIES
        IMPORTED_LOCATION
            "C:/libs/JoltPhysics/out/wasm-release/libJolt.a"
        INTERFACE_INCLUDE_DIRECTORIES
            "C:/libs/JoltPhysics"
    )
    set(SOL_EM_PORTS --use-port=emdawnwebgpu --use-port=contrib.glfw3)
    target_compile_options(sollib PRIVATE ${SOL_EM_PORTS})
    
    target_sources(solapp PRIVATE 
        ${CMAKE_SOURCE_DIR}/platform/web/web_main.cpp
        ${CMAKE_SOURCE_DIR}/platform/web/web_net.cpp
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
    find_package(glfw3 REQUIRED)
    add_library(webgpu SHARED IMPORTED)
    set_target_properties(webgpu PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "C:/libs/include"
        IMPORTED_IMPLIB "C:/libs/lib/wgpu_native.lib"
    )
    add_library(Jolt::Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt::Jolt PROPERTIES
        IMPORTED_LOCATION "C:/libs/JoltPhysics/out/windows-release/Jolt.lib"
        INTERFACE_INCLUDE_DIRECTORIES "C:/libs/JoltPhysics"
    )

    target_link_libraries(sollib PRIVATE glfw webgpu)

    if(WIN32)
    target_link_libraries(sollib PRIVATE
        ws2_32 bcrypt userenv ntdll advapi32 kernel32 user32 gdi32 shell32
    )
    endif()

    target_sources(solapp PRIVATE
        ${CMAKE_SOURCE_DIR}/platform/win/win_main.cpp
        ${CMAKE_SOURCE_DIR}/platform/win/win_net.cpp
    )
    target_link_libraries(solapp PRIVATE glfw ws2_32)
    set_target_properties(solapp PROPERTIES OUTPUT_NAME "SolApp")
endif()

target_link_libraries(sollib PRIVATE Jolt::Jolt glm::glm)
