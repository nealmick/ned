cmake_minimum_required(VERSION 3.10)
project(ned)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ========================
# Configuration & Settings
# ========================
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/util/config.h.in" "${CMAKE_BINARY_DIR}/config.h")
add_definitions(-DSOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(APPLE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(UNIX)
    add_compile_definitions(PLATFORM_LINUX)
endif()

# ==============
# Dependencies
# ==============
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)

# FreeType for better font rendering and emoji support
if(APPLE)
  # On macOS, use Homebrew's FreeType
  execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(ARCH STREQUAL "arm64")
    set(FREETYPE_INCLUDE_DIRS "/opt/homebrew/include/freetype2")
    set(FREETYPE_LIBRARIES "/opt/homebrew/lib/libfreetype.dylib")
  else()
    set(FREETYPE_INCLUDE_DIRS "/usr/local/include/freetype2")
    set(FREETYPE_LIBRARIES "/usr/local/lib/libfreetype.dylib")
  endif()
else()
  find_package(Freetype REQUIRED)
  set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS})
  set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARIES})
endif()

# GLEW Configuration
if(APPLE)
  execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(ARCH STREQUAL "arm64")
    set(GLEW_INCLUDE_DIRS "/opt/homebrew/Cellar/glew/2.2.0_1/include")
    set(GLEW_LIBRARIES "/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.dylib")
  else()
    set(GLEW_INCLUDE_DIRS "/usr/local/Cellar/glew/2.2.0_1/include")
    set(GLEW_LIBRARIES "/usr/local/Cellar/glew/2.2.0_1/lib/libGLEW.dylib")
  endif()
else()
  find_package(GLEW REQUIRED)
endif()

if(APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
  add_compile_options(-Wno-deprecated-declarations)
endif()

# ================
# Native File Dialog
# ================
set(NFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/nativefiledialog)
add_library(nfd STATIC)
target_sources(nfd PRIVATE
  ${NFD_DIR}/src/nfd_common.c
)

if(APPLE)
  target_sources(nfd PRIVATE ${NFD_DIR}/src/nfd_cocoa.m)
  target_compile_options(nfd PRIVATE -x objective-c -Wno-deprecated-declarations)
  target_link_libraries(nfd PRIVATE "-framework Cocoa")
elseif(UNIX)
  target_sources(nfd PRIVATE ${NFD_DIR}/src/nfd_gtk.c)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  target_include_directories(nfd PRIVATE ${GTK3_INCLUDE_DIRS})
  target_link_libraries(nfd PRIVATE ${GTK3_LIBRARIES})
endif()

target_include_directories(nfd PUBLIC
  ${NFD_DIR}/src
  ${NFD_DIR}/src/include
)

# =================
# Tree-Sitter Setup
# =================
# Core library
add_library(tree-sitter-lib STATIC
  lib/tree-sitter/lib/src/lib.c
  lib/tree-sitter/lib/src/parser.c
  lib/tree-sitter/lib/src/query.c 
)
target_include_directories(tree-sitter-lib PUBLIC
  lib/tree-sitter/lib/include
  lib/tree-sitter/lib/src
)

# Manually define grammars with non-standard structures
set(TS_GRAMMAR_LIBS)

# TypeScript
add_library(tree-sitter-typescript-grammar STATIC
  lib/tree-sitter-typescript/typescript/src/parser.c
  lib/tree-sitter-typescript/typescript/src/scanner.c
)
target_include_directories(tree-sitter-typescript-grammar PRIVATE
  lib/tree-sitter-typescript/typescript/src
)
list(APPEND TS_GRAMMAR_LIBS tree-sitter-typescript-grammar)

# TSX
add_library(tree-sitter-tsx-grammar STATIC
  lib/tree-sitter-typescript/tsx/src/parser.c
  lib/tree-sitter-typescript/tsx/src/scanner.c
)
target_include_directories(tree-sitter-tsx-grammar PRIVATE
  lib/tree-sitter-typescript/tsx/src
)
list(APPEND TS_GRAMMAR_LIBS tree-sitter-tsx-grammar)

# Other grammars with standard structure
file(GLOB TS_GRAMMAR_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/lib/tree-sitter-*")
foreach(TS_DIR ${TS_GRAMMAR_DIRS})
  get_filename_component(LANG ${TS_DIR} NAME)
  
  # Skip already added TypeScript grammars
  if(${LANG} MATCHES "typescript")
    continue()
  endif()

  string(REPLACE "tree-sitter-" "" LANG ${LANG})

  if(EXISTS "${TS_DIR}/src/scanner.c")
    set(GRAMMAR_SOURCES "${TS_DIR}/src/parser.c" "${TS_DIR}/src/scanner.c")
  else()
    set(GRAMMAR_SOURCES "${TS_DIR}/src/parser.c")
  endif()

  add_library(tree-sitter-${LANG}-grammar STATIC ${GRAMMAR_SOURCES})
  target_include_directories(tree-sitter-${LANG}-grammar PRIVATE ${TS_DIR}/src)
  set_target_properties(tree-sitter-${LANG}-grammar PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
  )
  target_link_libraries(tree-sitter-${LANG}-grammar PRIVATE tree-sitter-lib)
  list(APPEND TS_GRAMMAR_LIBS tree-sitter-${LANG}-grammar)
endforeach()

# ================
# Project Sources
# ================
# Core application
set(APP_SOURCES
  main.cpp
  ned.cpp
  shaders/shader.cpp
  shaders/shader_types.cpp
  shaders/shader_manager.cpp
)

# Component groups
file(GLOB_RECURSE EDITOR_SOURCES    "editor/*.cpp")
file(GLOB_RECURSE LSP_SOURCES       "lsp/*.cpp")
file(GLOB_RECURSE AI_SOURCES        "ai/*.cpp")
file(GLOB_RECURSE AI_MM_SOURCES     "ai/*.mm")
file(GLOB_RECURSE FILES_SOURCES     "files/*.cpp")

# ImGui sources (explicitly listed for stability)
set(IMGUI_SOURCES
  lib/imgui/imgui.cpp
  lib/imgui/imgui_demo.cpp
  lib/imgui/imgui_draw.cpp
  lib/imgui/imgui_tables.cpp
  lib/imgui/imgui_widgets.cpp
  lib/imgui/backends/imgui_impl_glfw.cpp
  lib/imgui/backends/imgui_impl_opengl3.cpp
  lib/imgui/misc/freetype/imgui_freetype.cpp
)

# Custom ImGui configuration to avoid modifying the submodule
add_definitions(-DIMGUI_USER_CONFIG="ned_imgui_config.h")

# Utility library
add_library(util STATIC
  util/settings.cpp
  util/settings_file_manager.cpp
  util/keybinds.cpp
  util/terminal.cpp
  util/close_popper.cpp
  util/welcome.cpp
  util/application_manager.cpp
  util/cleanup_manager.cpp
  util/debug_console.cpp
  util/font.cpp
  util/frame.cpp
  util/window_resize.cpp
  util/splitter.cpp
  util/init.cpp
  util/scroll.cpp
  util/render.cpp
  util/graphics_manager.cpp
  util/window_manager.cpp
)
target_include_directories(util PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  lib/imgui
  lib/imgui/backends
  editor
  files
  ${CMAKE_BINARY_DIR}
)
target_link_libraries(util PUBLIC ${OPENGL_LIBRARIES} glfw)

# ================
# Main Executable
# ================
add_executable(ned
  ${APP_SOURCES}
  ${EDITOR_SOURCES}
  ${LSP_SOURCES}
  ${AI_SOURCES}
  ${AI_MM_SOURCES}
  ${FILES_SOURCES}
  ${IMGUI_SOURCES}
)

# =====================
# macOS Window Management
# =====================
if(APPLE)
  # Find GLFW headers using standard CMake path
  find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
    PATHS
    /usr/local/include
    /opt/homebrew/include
    ${GLFW3_INCLUDE_DIRS}
  )
  
  # Add Objective-C++ support
  set(CMAKE_OBJCXX_STANDARD 11)
  add_library(macos_window OBJECT macos_window.mm)
  
  # Add GLFW includes with verified path
  target_include_directories(macos_window PRIVATE
    ${GLFW_INCLUDE_DIR}
  )
  
  target_sources(ned PRIVATE $<TARGET_OBJECTS:macos_window>)
  
endif()


# =====================
# Include Directories
# =====================
target_include_directories(ned PRIVATE
  lib/imgui
  lib/imgui/backends
  ${NFD_DIR}/src
  ${NFD_DIR}/src/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  editor
  files
  ai
  ai/mcp
  ${CMAKE_BINARY_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  lib/tree-sitter/lib/include
  lib/tree-sitter/lib/src
  lib/utfcpp/source
)

# ================
# Linking
# ================
target_link_libraries(ned PRIVATE
  ${OPENGL_LIBRARIES}
  glfw
  nfd
  util
  ${GLEW_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  CURL::libcurl 
  tree-sitter-lib
  ${TS_GRAMMAR_LIBS}
)

if(APPLE)
  target_link_libraries(ned PRIVATE
    "-framework Cocoa"
    "-framework QuartzCore" 
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework OpenGL"
  )
endif()

# ================
# Resources
# ================
file(COPY 
  ${CMAKE_SOURCE_DIR}/settings
  ${CMAKE_SOURCE_DIR}/fonts
  ${CMAKE_SOURCE_DIR}/icons
  ${CMAKE_SOURCE_DIR}/shaders
  ${CMAKE_SOURCE_DIR}/editor/queries 
  DESTINATION ${CMAKE_BINARY_DIR}
)