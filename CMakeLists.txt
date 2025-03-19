cmake_minimum_required(VERSION 3.10)
project(ned)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/util/config.h.in"
    "${CMAKE_BINARY_DIR}/config.h"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Explicitly set GLEW paths
# Find GLEW properly
if(APPLE)
  # Check if we're on Apple Silicon (M-series) or Intel
  execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(${ARCH} STREQUAL "arm64")
    # M-series Mac
    set(GLEW_INCLUDE_DIRS "/opt/homebrew/Cellar/glew/2.2.0_1/include")
    set(GLEW_LIBRARIES "/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.dylib")
  else()
    # Intel Mac
    set(GLEW_INCLUDE_DIRS "/usr/local/Cellar/glew/2.2.0_1/include")
    set(GLEW_LIBRARIES "/usr/local/Cellar/glew/2.2.0_1/lib/libGLEW.dylib")
  endif()
else()
  # For non-Apple systems, use find_package
  find_package(GLEW REQUIRED)
endif()


if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    add_compile_options(-Wno-deprecated-declarations)
endif()

# NFD setup
set(NFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/nativefiledialog)
if(APPLE)
    add_library(nfd STATIC
        ${NFD_DIR}/src/nfd_common.c
        ${NFD_DIR}/src/nfd_cocoa.m
    )
    target_compile_options(nfd PRIVATE -x objective-c -Wno-deprecated-declarations)
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
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Add util library
add_library(util STATIC
    util/settings.cpp
    util/terminal.cpp
    util/close_popper.cpp
    util/welcome.cpp
    util/debug_console.cpp
)
add_definitions(-DSOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/editor  # Add editor directory
    ${CMAKE_CURRENT_SOURCE_DIR}/files   # Add files directory
    ${CMAKE_BINARY_DIR}  # This is where config.h gets generated
)

target_link_libraries(util PUBLIC
    ${OPENGL_LIBRARIES}
    glfw
)

# Main executable
add_executable(ned
    main.cpp
    ned.cpp
    shaders/shader.cpp

    # Editor files
    editor/editor.cpp
    editor/editor_line_jump.cpp
    editor/editor_scroll.cpp
    editor/editor_cursor.cpp
    editor/editor_copy_paste.cpp
    editor/editor_indentation.cpp
    editor/editor_line_numbers.cpp
    editor/editor_highlight.cpp
    editor/editor_mouse.cpp
    editor/editor_keyboard.cpp
    editor/editor_selection.cpp
    editor/editor_render.cpp

    lsp/lsp.cpp
    lsp/lsp_adapter_clangd.cpp
    lsp/lsp_adapter_pyright.cpp
    lsp/lsp_manager.cpp
    
    editor/editor_types.h
    editor/editor_utils.h


    # Files
    files/files.cpp
    files/file_tree.cpp
    files/file_finder.cpp
    files/file_content_search.cpp
    
    # ImGui files
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(ned PRIVATE
    lib/imgui
    lib/imgui/backends
    ${NFD_DIR}/src
    ${NFD_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/editor  # Add editor directory
    ${CMAKE_CURRENT_SOURCE_DIR}/files   # Add files directory
    ${CMAKE_BINARY_DIR}
    ${GLEW_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(ned PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    nfd
    util
    ${GLEW_LIBRARIES}
)

if(APPLE)
    target_link_libraries(ned PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
    )
endif()

