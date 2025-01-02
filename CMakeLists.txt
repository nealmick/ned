cmake_minimum_required(VERSION 3.10)
project(ned)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/util/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# NFD setup
set(NFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog)
if(APPLE)
    add_library(nfd STATIC
        ${NFD_DIR}/src/nfd_common.c
        ${NFD_DIR}/src/nfd_cocoa.m
    )
    target_compile_options(nfd PRIVATE -x objective-c)
    target_link_libraries(nfd PRIVATE "-framework Cocoa")
elseif(UNIX)
    add_library(nfd STATIC
        ${NFD_DIR}/src/nfd_common.c
        ${NFD_DIR}/src/nfd_gtk.c
    )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(nfd PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(nfd PRIVATE ${GTK3_LIBRARIES})
endif()

target_include_directories(nfd PUBLIC 
    ${NFD_DIR}/src
    ${NFD_DIR}/src/include
)

# Copy resources
file(COPY ${CMAKE_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/icons DESTINATION ${CMAKE_BINARY_DIR})

# Add util library
add_library(util STATIC
    util/line_jump.cpp
)

target_include_directories(util PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

target_link_libraries(util PUBLIC
    ${OPENGL_LIBRARIES}
    glfw
)

# Main executable
add_executable(ned
    main.cpp
    util/settings.cpp
    util/terminal.cpp   
    files.cpp
    editor.cpp
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(ned PRIVATE 
    imgui 
    imgui/backends 
    ${NFD_DIR}/src
    ${NFD_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link libraries
target_link_libraries(ned PRIVATE 
    ${OPENGL_LIBRARIES} 
    glfw 
    nfd
    util
)

if(APPLE)
    target_link_libraries(ned PRIVATE 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreVideo"
    )
endif()