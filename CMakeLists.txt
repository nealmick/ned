cmake_minimum_required(VERSION 3.10)
project(ned)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ========================
# Configuration & Settings
# ========================
add_definitions(-DSOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optional features (default ON — full product). Pass -DNED_ENABLE_*=OFF for lean builds.
option(NED_ENABLE_TERMINAL "Embedded terminal (imgui-terminal)" ON)
option(NED_ENABLE_GIT      "Git gutter/status via libgit2" ON)
option(NED_ENABLE_LSP      "Language Server Protocol client" ON)
option(NED_ENABLE_SHADERS  "CRT/burn-in postprocess (standalone ned)" ON)

function(ned_apply_feature_defs target)
  if(NED_ENABLE_TERMINAL)
    target_compile_definitions(${target} PUBLIC NED_ENABLE_TERMINAL=1)
  else()
    target_compile_definitions(${target} PUBLIC NED_ENABLE_TERMINAL=0)
  endif()
  if(NED_ENABLE_GIT)
    target_compile_definitions(${target} PUBLIC NED_ENABLE_GIT=1)
  else()
    target_compile_definitions(${target} PUBLIC NED_ENABLE_GIT=0)
  endif()
  if(NED_ENABLE_LSP)
    target_compile_definitions(${target} PUBLIC NED_ENABLE_LSP=1)
  else()
    target_compile_definitions(${target} PUBLIC NED_ENABLE_LSP=0)
  endif()
  if(NED_ENABLE_SHADERS)
    target_compile_definitions(${target} PUBLIC NED_ENABLE_SHADERS=1)
  else()
    target_compile_definitions(${target} PUBLIC NED_ENABLE_SHADERS=0)
  endif()
endfunction()

message(STATUS "Ned features: TERMINAL=${NED_ENABLE_TERMINAL} GIT=${NED_ENABLE_GIT} LSP=${NED_ENABLE_LSP} SHADERS=${NED_ENABLE_SHADERS}")

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
        # Source + execution charset UTF-8. Without this, MSVC treats .cpp as the
        # system code page and u8"é"/emoji literals become mojibake (C4099-level
        # breakage in UTF-8 unit tests and any non-ASCII string literals).
        add_compile_options(/utf-8)
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
# libgit2 Configuration (optional — NED_ENABLE_GIT)
# ================
if(NED_ENABLE_GIT)
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
# Tree-Sitter Setup (vendored under lib/treesitter/)
# =================
set(TS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/lib/treesitter)

# Core library
add_library(tree-sitter-lib STATIC
  ${TS_ROOT}/tree-sitter/lib/src/lib.c
  ${TS_ROOT}/tree-sitter/lib/src/query.c
)
target_include_directories(tree-sitter-lib PUBLIC
  ${TS_ROOT}/tree-sitter/lib/include
  ${TS_ROOT}/tree-sitter/lib/src
)

# Manually define grammars with non-standard structures
set(TS_GRAMMAR_LIBS)

# TypeScript
add_library(tree-sitter-typescript-grammar STATIC
  ${TS_ROOT}/tree-sitter-typescript/typescript/src/parser.c
  ${TS_ROOT}/tree-sitter-typescript/typescript/src/scanner.c
)
target_include_directories(tree-sitter-typescript-grammar PRIVATE
  ${TS_ROOT}/tree-sitter-typescript/typescript/src
)
list(APPEND TS_GRAMMAR_LIBS tree-sitter-typescript-grammar)

# TSX
add_library(tree-sitter-tsx-grammar STATIC
  ${TS_ROOT}/tree-sitter-typescript/tsx/src/parser.c
  ${TS_ROOT}/tree-sitter-typescript/tsx/src/scanner.c
)
target_include_directories(tree-sitter-tsx-grammar PRIVATE
  ${TS_ROOT}/tree-sitter-typescript/tsx/src
)
list(APPEND TS_GRAMMAR_LIBS tree-sitter-tsx-grammar)

# Other grammars with standard structure
file(GLOB TS_GRAMMAR_DIRS "${TS_ROOT}/tree-sitter-*")
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
  workbench.cpp
)
if(NED_ENABLE_SHADERS)
  list(APPEND APP_SOURCES
    shaders/shader.cpp
    shaders/shader_types.cpp
    shaders/shader_manager.cpp
  )
endif()

# Component groups
file(GLOB_RECURSE EDITOR_SOURCES "editor/*.cpp")
if(NED_ENABLE_GIT)
  # Drop stub if present so real implementation is the only TU.
  list(FILTER EDITOR_SOURCES EXCLUDE REGEX ".*/git_service_stub\\.cpp$")
else()
  list(FILTER EDITOR_SOURCES EXCLUDE REGEX ".*/services/git/.*")
  list(APPEND EDITOR_SOURCES editor/services/git/git_service_stub.cpp)
endif()

file(GLOB_RECURSE FILES_SOURCES "files/*.cpp")

# LSP (optional — NED_ENABLE_LSP)
set(LSP_SOURCES)
if(NED_ENABLE_LSP)
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
else()
  set(LSP_SOURCES lsp/lsp_client_stub.cpp)
endif()

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

# ================
# ImGui-Terminal (optional — NED_ENABLE_TERMINAL)
# ================
set(IMGUI_TERMINAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui-terminal)
if(NED_ENABLE_TERMINAL)
  add_library(imgui_terminal_widget STATIC
    ${IMGUI_TERMINAL_DIR}/terminal.cpp
  )
  target_include_directories(imgui_terminal_widget PUBLIC
    ${IMGUI_TERMINAL_DIR}
  )
  target_include_directories(imgui_terminal_widget PRIVATE
    ${IMGUI_INCLUDE_DIRS}
    lib/imgui/misc/freetype
    ${CMAKE_CURRENT_SOURCE_DIR} # ned_imgui_config.h via IMGUI_USER_CONFIG
  )
  target_compile_definitions(imgui_terminal_widget PRIVATE
    IMGUI_USER_CONFIG="ned_imgui_config.h"
  )
  if(UNIX)
    target_compile_definitions(imgui_terminal_widget PRIVATE _DEFAULT_SOURCE)
  endif()
  if(WIN32)
    target_compile_definitions(imgui_terminal_widget PRIVATE
      _WIN32_WINNT=0x0A00 WINVER=0x0A00 NTDDI_VERSION=0x0A000006
      WIN32_LEAN_AND_MEAN)
    if(MSVC)
      target_compile_definitions(imgui_terminal_widget PRIVATE _CRT_SECURE_NO_WARNINGS)
      target_compile_options(imgui_terminal_widget PRIVATE /utf-8)
    endif()
  endif()

  # Fontconfig (terminal glyph discovery) + FreeType
  if(MSVC)
    find_package(Fontconfig REQUIRED)
    target_link_libraries(imgui_terminal_widget PUBLIC
      Fontconfig::Fontconfig Freetype::Freetype)
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
    target_link_libraries(imgui_terminal_widget PUBLIC
      PkgConfig::FONTCONFIG Freetype::Freetype)
  endif()

  # openpty lives in libutil on Linux; on macOS it is in libSystem.
  if(UNIX AND NOT APPLE)
    find_library(LIBUTIL_LIBRARY NAMES util libutil)
    if(LIBUTIL_LIBRARY)
      target_link_libraries(imgui_terminal_widget PUBLIC ${LIBUTIL_LIBRARY})
    endif()
  endif()

  # rgb.txt next to the binary (and cwd) for X11 color-name lookup
  file(COPY ${IMGUI_TERMINAL_DIR}/rgb.txt DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Utility library
set(NED_UTIL_SOURCES
  util/settings.cpp
  util/keybinds.cpp
  util/welcome.cpp
  util/font.cpp
  util/icons.cpp
  util/project_undo.cpp
)
if(NED_ENABLE_TERMINAL)
  list(APPEND NED_UTIL_SOURCES util/ned_terminal.cpp)
else()
  list(APPEND NED_UTIL_SOURCES util/ned_terminal_stub.cpp)
endif()

add_library(ned_util STATIC ${NED_UTIL_SOURCES})
ned_apply_feature_defs(ned_util)
target_include_directories(ned_util PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${IMGUI_INCLUDE_DIRS}
  editor
  files
  ${CMAKE_BINARY_DIR}
  lib/imgui/misc/freetype
  # settings / welcome touch editor + tree-sitter headers
  ${TS_ROOT}/tree-sitter/lib/include
  ${TS_ROOT}/tree-sitter/lib/src
)
if(NED_ENABLE_LSP)
  target_include_directories(ned_util PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/lib/lsp-framework/generated
    lib/lsp-framework
  )
endif()
if(NED_ENABLE_GIT)
  target_include_directories(ned_util PUBLIC lib/libgit2/include)
endif()
if(NED_ENABLE_TERMINAL)
  target_include_directories(ned_util PUBLIC ${IMGUI_TERMINAL_DIR})
endif()

set(NED_UTIL_LINK_LIBS ${OPENGL_LIBRARIES} glfw)
if(NED_ENABLE_GIT)
  list(APPEND NED_UTIL_LINK_LIBS libgit2 util)
endif()
if(NED_ENABLE_TERMINAL)
  list(APPEND NED_UTIL_LINK_LIBS imgui_terminal_widget)
endif()
target_link_libraries(ned_util PUBLIC ${NED_UTIL_LINK_LIBS})
if(NOT WIN32)
  target_link_libraries(ned_util PUBLIC Freetype::Freetype)
endif()
set(NED_UTIL_DEPS tree-sitter-lib)
if(NED_ENABLE_GIT)
  list(APPEND NED_UTIL_DEPS libgit2 util)
endif()
if(NED_ENABLE_LSP)
  # settings include lsp headers that need generated types.
  list(APPEND NED_UTIL_DEPS lsp)
endif()
add_dependencies(ned_util ${NED_UTIL_DEPS})

# ================
# Main Executable (only when ned is the top-level project)
# ================
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if(WIN32)
    # Windows icon + VERSIONINFO. Configure absolute .ico path so rc.exe always
    # finds the file (relative paths break depending on MSVC cwd).
    # Do not use IDI_APPLICATION — after windows.h it is MAKEINTRESOURCE(32512)
    # and is a poor choice for the shell's primary app icon.
    set(NED_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ned.ico")
    # RC accepts forward slashes; quote the path in the .rc itself.
    file(TO_CMAKE_PATH "${NED_ICON_PATH}" NED_ICON_PATH)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/ned.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/ned.rc
      @ONLY
    )
    set(WINDOWS_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/ned.rc)
  endif()

  add_executable(ned
    ${APP_SOURCES}
    ${EDITOR_SOURCES}
    ${LSP_SOURCES}
    ${FILES_SOURCES}
    ${IMGUI_SOURCES}
    ${WINDOWS_RESOURCES}
  )
  ned_apply_feature_defs(ned)
  if(WIN32)
    # WINDOWS subsystem (no console). Keep main() — MSVC defaults to WinMain.
    set_target_properties(ned PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
      target_link_options(ned PRIVATE /ENTRY:mainCRTStartup)
    endif()
  endif()
endif()

# ================
# Embeddable Library
# ================
add_library(ned_embed STATIC
  ned_embed.cpp
  workbench.cpp
  ${EDITOR_SOURCES}
  ${LSP_SOURCES}
  ${FILES_SOURCES}
  ${IMGUI_SOURCES}
)
ned_apply_feature_defs(ned_embed)

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
  add_library(macos_window OBJECT util/macos_window.mm)
  
  # Add GLFW includes with verified path
  target_include_directories(macos_window PRIVATE
    ${GLFW_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
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
    ${CMAKE_BINARY_DIR}
    $<TARGET_PROPERTY:GLEW::GLEW,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>
    ${TS_ROOT}/tree-sitter/lib/include
    ${TS_ROOT}/tree-sitter/lib/src
    ${GLFW_INCLUDE_DIR}
  )
  if(NED_ENABLE_LSP)
    target_include_directories(ned PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/lib/lsp-framework/generated
      lib/lsp-framework
    )
  endif()
  if(NED_ENABLE_GIT)
    target_include_directories(ned PRIVATE lib/libgit2/include)
  endif()
endif()

target_include_directories(ned_embed PUBLIC
  ${IMGUI_INCLUDE_DIRS}
  ${NFD_DIR}/src
  ${NFD_DIR}/src/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  editor
  files
  ${CMAKE_BINARY_DIR}
  $<TARGET_PROPERTY:GLEW::GLEW,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>
  ${TS_ROOT}/tree-sitter/lib/include
  ${TS_ROOT}/tree-sitter/lib/src
  ${GLFW_INCLUDE_DIR}
)
if(NED_ENABLE_LSP)
  target_include_directories(ned_embed PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/lib/lsp-framework/generated
    lib/lsp-framework
  )
endif()
if(NED_ENABLE_GIT)
  target_include_directories(ned_embed PUBLIC lib/libgit2/include)
endif()

# ================
# Linking
# ================
# Feature-optional object/static libs (libgit2 stack, lsp)
set(NED_FEATURE_LIBS)
set(NED_FEATURE_DEPS)
if(NED_ENABLE_GIT)
  list(APPEND NED_FEATURE_LIBS libgit2 util llhttp xdiff pcre)
  list(APPEND NED_FEATURE_DEPS libgit2 util llhttp xdiff)
endif()
if(NED_ENABLE_LSP)
  list(APPEND NED_FEATURE_LIBS lsp)
endif()

# Platform extras mostly needed for libgit2 HTTPS / compression
set(NED_PLATFORM_LIBS
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

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
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
    ${NED_FEATURE_LIBS}
    ${NED_PLATFORM_LIBS}
  )
  if(NED_FEATURE_DEPS)
    add_dependencies(ned ${NED_FEATURE_DEPS})
  endif()
endif()

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
    ${NED_FEATURE_LIBS}
    ${NED_PLATFORM_LIBS}
  )
  if(NED_FEATURE_DEPS)
    add_dependencies(ned_embed ${NED_FEATURE_DEPS})
  endif()
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
    ${NED_FEATURE_LIBS}
    ${NED_PLATFORM_LIBS}
  )
  if(NED_FEATURE_DEPS)
    add_dependencies(ned_embed ${NED_FEATURE_DEPS})
  endif()
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
set(NED_RESOURCE_COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/resources
  ${CMAKE_CURRENT_SOURCE_DIR}/editor/services/highlight/queries
)
if(NED_ENABLE_SHADERS)
  list(APPEND NED_RESOURCE_COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
endif()
file(COPY ${NED_RESOURCE_COPY} DESTINATION ${CMAKE_BINARY_DIR})

# ================
# Unit tests (Monaco-shaped editor model suite)
# ================
option(NED_BUILD_TESTS "Build ned unit tests (Monaco model ports)" ON)
if(NED_BUILD_TESTS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  enable_testing()
  add_subdirectory(tests)
endif()
