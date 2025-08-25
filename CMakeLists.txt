cmake_minimum_required(VERSION 3.10)
project(ned)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ========================
# Configuration & Settings
# ========================
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/util/config.h.in" "${CMAKE_BINARY_DIR}/config.h")
add_definitions(-DSOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(APPLE)
    add_compile_definitions(PLATFORM_MACOS)
    # Set minimum macOS version to 11.0 (Big Sur)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
    # Detect current architecture and target only that
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architecture for macOS" FORCE)
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architecture for macOS" FORCE)
    endif()
    # Add compatibility flags for macOS 11.0+
    add_compile_options(-mmacosx-version-min=11.0)
    add_link_options(-mmacosx-version-min=11.0)
elseif(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(_WIN32_WINNT=0x0601) # Windows 7+
    # MSVC-specific flags from lsp-framework documentation
    if(MSVC)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        add_compile_definitions(UTF_CPP_CPLUSPLUS=202002L)
        add_compile_options(/std:c++20)
        # Only add essential C++20 recognition flag
        add_compile_options(/Zc:__cplusplus)
        # Enable parallel compilation for faster builds
        add_compile_options(/MP)
    endif()
elseif(UNIX)
    add_compile_definitions(PLATFORM_LINUX)
    # Ensure proper feature test macros for Linux to expose le16toh/be16toh functions
    add_compile_definitions(_GNU_SOURCE)
    add_compile_definitions(_DEFAULT_SOURCE)
endif()

# ==============
# Dependencies
# ==============
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)

# FreeType for better font rendering and emoji support
find_package(Freetype REQUIRED)

# GLEW Configuration
find_package(GLEW REQUIRED)

if(APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
  add_compile_options(-Wno-deprecated-declarations)
endif()

# ================
# libgit2 Configuration  
# ================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libgit2 as static library")
set(BUILD_TESTS OFF CACHE BOOL "Disable libgit2 tests")
set(BUILD_CLI OFF CACHE BOOL "Disable libgit2 CLI tools")
set(USE_SSH OFF CACHE BOOL "Disable SSH support")
set(USE_HTTPS ON CACHE BOOL "Enable HTTPS support")
set(USE_NTLMCLIENT OFF CACHE BOOL "Disable NTLM client")
set(REGEX_BACKEND "builtin" CACHE STRING "Force libgit2 to use builtin regex")
set(USE_BUNDLED_ZLIB ON CACHE BOOL "Use bundled zlib")
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries")
set(EMBED_SSH_PATH OFF CACHE BOOL "Disable SSH path")
set(THREADSAFE OFF CACHE BOOL "Disable threadsafe")
add_subdirectory(lib/libgit2)

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
elseif(WIN32)
  target_sources(nfd PRIVATE ${NFD_DIR}/src/nfd_win.cpp)
  target_link_libraries(nfd PRIVATE shell32 ole32 uuid)
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
file(GLOB_RECURSE AI_SOURCES        "ai/*.cpp")
file(GLOB_RECURSE AI_MM_SOURCES     "ai/*.mm")
file(GLOB_RECURSE FILES_SOURCES     "files/*.cpp")

# Add lsp-framework subdirectory with sanitizers disabled
set(LSP_USE_SANITIZERS OFF CACHE BOOL "Disable sanitizers in lsp-framework" FORCE)
add_subdirectory(lib/lsp-framework)

# Apply MSVC-specific flags only to lsp-framework on Windows
if(WIN32 AND MSVC)
    target_compile_options(lsp PRIVATE /permissive-)
    target_compile_definitions(lsp PRIVATE _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH)
endif()

# Replace problematic connection.cpp with our patched version for all platforms
get_target_property(LSP_SOURCES_EXISTING lsp SOURCES)
list(REMOVE_ITEM LSP_SOURCES_EXISTING ${CMAKE_CURRENT_SOURCE_DIR}/lib/lsp-framework/lsp/connection.cpp)
list(APPEND LSP_SOURCES_EXISTING ${CMAKE_CURRENT_SOURCE_DIR}/lsp/connection_patched.cpp)
set_target_properties(lsp PROPERTIES SOURCES "${LSP_SOURCES_EXISTING}")

set(LSP_SOURCES
  lsp/lsp_client.cpp
  lsp/lsp_symbol_info.cpp
  lsp/lsp_goto_def.cpp
  lsp/lsp_goto_ref.cpp
  lsp/lsp_uri_options.cpp
  lsp/lsp_dashboard.cpp
)

# Find GLFW headers
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
  PATHS
  /usr/local/include
  /opt/homebrew/include
  ${GLFW3_INCLUDE_DIRS}
)

# ImGui sources (explicitly listed for stability)
# If building as a subdirectory (embedded), do NOT add or build ImGui, FreeType, or GLEW.
# Just expect the parent to provide those libraries and include directories.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # Standalone build - use our own ImGui
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
    set(IMGUI_INCLUDE_DIRS lib/imgui lib/imgui/backends)
else()
    # Embedded build - expect parent to provide ImGui, FreeType, GLEW, etc.
    set(IMGUI_SOURCES)
    set(IMGUI_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/imgui/backends)
endif()

# Custom ImGui configuration to avoid modifying the submodule
add_definitions(-DIMGUI_USER_CONFIG="ned_imgui_config.h")

# Utility library
add_library(ned_util STATIC
  util/settings.cpp
  util/settings_file_manager.cpp
  util/keybinds.cpp
  util/terminal.cpp
  util/close_popper.cpp
  util/welcome.cpp
  util/app.cpp
  util/debug_console.cpp
  util/font.cpp
  util/window_resize.cpp
  util/splitter.cpp
  util/init.cpp
  util/scroll.cpp
  util/render.cpp
)
target_include_directories(ned_util PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${IMGUI_INCLUDE_DIRS}
  editor
  files
  ${CMAKE_BINARY_DIR}
  lib/libgit2/include
)
target_link_libraries(ned_util PUBLIC ${OPENGL_LIBRARIES} glfw libgit2 util)
add_dependencies(ned_util libgit2 util)

# ================
# Main Executable (only in standalone mode)
# ================
# Add Windows resource file
if(WIN32)
  set(WINDOWS_RESOURCES ned.rc)
endif()

# Force ned executable to be built
add_executable(ned
  globals.cpp
  ${APP_SOURCES}
  ${EDITOR_SOURCES}
  ${LSP_SOURCES}
  ${AI_SOURCES}
  ${AI_MM_SOURCES}
  ${FILES_SOURCES}
  ${IMGUI_SOURCES}
  ${WINDOWS_RESOURCES}
)

# ================
# Embeddable Library
# ================
add_library(ned_embed STATIC
  globals.cpp
  ned_embed.cpp
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
  
  if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    target_sources(ned PRIVATE $<TARGET_OBJECTS:macos_window>)
  endif()
  
endif()


# =====================
# Include Directories
# =====================
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  target_include_directories(ned PRIVATE
    ${IMGUI_INCLUDE_DIRS}
    ${NFD_DIR}/src
    ${NFD_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    editor
    files
    ai
    ai/mcp
    ${CMAKE_BINARY_DIR}
    $<TARGET_PROPERTY:GLEW::GLEW,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>
    lib/tree-sitter/lib/include
    lib/tree-sitter/lib/src
    lib/utfcpp/source
    lib/libgit2/include
    ${GLFW_INCLUDE_DIR}
  )
endif()

target_include_directories(ned_embed PUBLIC
  ${IMGUI_INCLUDE_DIRS}
  ${NFD_DIR}/src
  ${NFD_DIR}/src/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  editor
  files
  ai
  ai/mcp
  ${CMAKE_BINARY_DIR}
  $<TARGET_PROPERTY:GLEW::GLEW,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>
  lib/tree-sitter/lib/include
  lib/tree-sitter/lib/src
  lib/utfcpp/source
  lib/libgit2/include
  ${GLFW_INCLUDE_DIR}
)

# ================
# Linking
# ================
# Force standalone linking
target_link_libraries(ned PRIVATE
  ${OPENGL_LIBRARIES}
  glfw
  nfd
  ned_util
  GLEW::GLEW
  Freetype::Freetype
  CURL::libcurl 
  tree-sitter-lib
  ${TS_GRAMMAR_LIBS}
  libgit2
  util
  llhttp
  xdiff
  pcre
  lsp
  $<$<PLATFORM_ID:Windows>:ZLIB::ZLIB>
  $<$<PLATFORM_ID:Darwin>:z>
  $<$<PLATFORM_ID:Linux>:z>
  $<$<PLATFORM_ID:Darwin>:iconv>
  $<$<PLATFORM_ID:Linux>:pthread>
  $<$<PLATFORM_ID:Linux>:ssl>
  $<$<PLATFORM_ID:Linux>:crypto>
  $<$<PLATFORM_ID:Linux>:pcre>
  $<$<PLATFORM_ID:Linux>:rt>
)
add_dependencies(ned libgit2 util llhttp xdiff)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # Standalone: link to our own ImGui, FreeType, GLEW, etc.
  target_link_libraries(ned_embed PUBLIC
    ${OPENGL_LIBRARIES}
    glfw
    nfd
    ned_util
    GLEW::GLEW
    Freetype::Freetype
    CURL::libcurl 
    tree-sitter-lib
    ${TS_GRAMMAR_LIBS}
    libgit2
    util
    llhttp
    xdiff
    pcre
    lsp
    $<$<PLATFORM_ID:Windows>:ZLIB::ZLIB>
  $<$<PLATFORM_ID:Darwin>:z>
  $<$<PLATFORM_ID:Linux>:z>
    $<$<PLATFORM_ID:Darwin>:iconv>
    $<$<PLATFORM_ID:Linux>:pthread>
    $<$<PLATFORM_ID:Linux>:ssl>
    $<$<PLATFORM_ID:Linux>:crypto>
    $<$<PLATFORM_ID:Linux>:pcre>
    $<$<PLATFORM_ID:Linux>:rt>
  )
  add_dependencies(ned_embed libgit2 util llhttp xdiff)
else()
  # Embedded: expect parent to provide ImGui, FreeType, GLEW, etc.
  # Note: imgui is compiled directly into the parent executable, so we don't link against it
    target_link_libraries(ned_embed PUBLIC
    OpenGL::GL
    glfw
    nfd
    ned_util
    GLEW::GLEW
    Freetype::Freetype
    CURL::libcurl 
    tree-sitter-lib
    ${TS_GRAMMAR_LIBS}
    libgit2
    util
    llhttp
    xdiff
    pcre
    lsp
    $<$<PLATFORM_ID:Windows>:ZLIB::ZLIB>
  $<$<PLATFORM_ID:Darwin>:z>
  $<$<PLATFORM_ID:Linux>:z>
  $<$<PLATFORM_ID:Darwin>:iconv>
  $<$<PLATFORM_ID:Linux>:pthread>
  $<$<PLATFORM_ID:Linux>:ssl>
  $<$<PLATFORM_ID:Linux>:crypto>
  $<$<PLATFORM_ID:Linux>:pcre>
  $<$<PLATFORM_ID:Linux>:rt>
  )
  add_dependencies(ned_embed libgit2 util llhttp xdiff)
endif()

if(APPLE AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  target_link_libraries(ned PRIVATE
    "-framework Cocoa"
    "-framework CoreServices"
    "-framework QuartzCore" 
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework OpenGL"
    "-framework Security"
    "-framework GSS"
    "-framework CoreFoundation"
  )
endif()

if(WIN32 AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  target_link_libraries(ned PRIVATE
    winhttp
    secur32
    crypt32
    rpcrt4
    ws2_32
    advapi32
    shell32
    ole32
    oleaut32
    user32
    comdlg32
  )
endif()

# Add macOS frameworks for ned_embed when building on Apple platforms
if(APPLE)
  target_link_libraries(ned_embed PUBLIC
    "-framework Security"
    "-framework GSS"
    "-framework CoreFoundation"
  )
endif()

# ================
# Resources
# ================
file(COPY 
  ${CMAKE_CURRENT_SOURCE_DIR}/settings
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts
  ${CMAKE_CURRENT_SOURCE_DIR}/icons
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders
  ${CMAKE_CURRENT_SOURCE_DIR}/editor/queries 
  DESTINATION ${CMAKE_BINARY_DIR}
)