# ned unit tests
#   - Monaco-shaped document model suite (EditorState + EditorOperations)
#   - Layer suite: commands, undo, save, git (if NED_ENABLE_GIT), UTF-8 helpers
#
# Build: cmake -DNED_BUILD_TESTS=ON ...
# Run:   ctest --test-dir <build> -R ned_tests --output-on-failure
#    or: ./tests/ned_tests

# ---------------------------------------------------------------------------
# Core document model (existing Monaco ports + ops tests)
# ---------------------------------------------------------------------------
set(NED_MODEL_SOURCES
  ${CMAKE_SOURCE_DIR}/editor/buffer/text_buffer.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_state.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_operations.cpp
  monaco/text_model.cpp
)

add_library(ned_editor_model STATIC ${NED_MODEL_SOURCES})
target_include_directories(ned_editor_model PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/editor
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(ned_editor_model PUBLIC cxx_std_20)
ned_apply_feature_defs(ned_editor_model)

# ---------------------------------------------------------------------------
# Layer library: production commands / undo / save / view
# ---------------------------------------------------------------------------
set(NED_LAYER_SOURCES
  ${CMAKE_SOURCE_DIR}/editor/buffer/text_buffer.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_state.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_operations.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_view_state.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_commands.cpp
  ${CMAKE_SOURCE_DIR}/editor/editor_events.cpp
  ${CMAKE_SOURCE_DIR}/util/project_undo.cpp
  ${CMAKE_SOURCE_DIR}/editor/services/save_service.cpp
  # ImGui core + FreeType (matches app IMGUI_USER_CONFIG); no GLFW backend.
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/misc/freetype/imgui_freetype.cpp
)
if(NED_ENABLE_GIT)
  list(APPEND NED_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/editor/services/git/git_repo.cpp
    ${CMAKE_SOURCE_DIR}/editor/services/git/line_diff.cpp
  )
endif()

add_library(ned_editor_layer STATIC ${NED_LAYER_SOURCES})
target_include_directories(ned_editor_layer PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/editor
  ${CMAKE_SOURCE_DIR}/lib/imgui
  ${CMAKE_SOURCE_DIR}/lib/imgui/misc/freetype
  ${CMAKE_CURRENT_SOURCE_DIR}
)
if(NED_ENABLE_GIT)
  target_include_directories(ned_editor_layer PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/libgit2/include
  )
endif()
# editor_utils.h pulls GLFW for rainbow helper; keep include path available.
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
  PATHS /usr/local/include /opt/homebrew/include ${GLFW3_INCLUDE_DIRS}
)
if(GLFW_INCLUDE_DIR)
  target_include_directories(ned_editor_layer PUBLIC ${GLFW_INCLUDE_DIR})
endif()
find_package(Freetype REQUIRED)

set(NED_LAYER_LIBS Freetype::Freetype)
if(NED_ENABLE_GIT)
  # libgit2 is linked as object targets (libgit2 + util + …), not libgit2package.
  list(APPEND NED_LAYER_LIBS
    libgit2
    util
    llhttp
    xdiff
    pcre
    $<$<PLATFORM_ID:Darwin>:z>
    $<$<PLATFORM_ID:Darwin>:iconv>
    $<$<PLATFORM_ID:Linux>:z>
    $<$<PLATFORM_ID:Linux>:pthread>
    $<$<PLATFORM_ID:Linux>:ssl>
    $<$<PLATFORM_ID:Linux>:crypto>
  )
endif()
target_link_libraries(ned_editor_layer PUBLIC ${NED_LAYER_LIBS})

if(NED_ENABLE_GIT AND APPLE)
  target_link_libraries(ned_editor_layer PUBLIC
    "-framework Security"
    "-framework GSS"
    "-framework CoreFoundation"
  )
endif()
if(NED_ENABLE_GIT AND WIN32)
  if(TARGET zlib)
    target_sources(ned_editor_layer PRIVATE $<TARGET_OBJECTS:zlib>)
  else()
    find_package(ZLIB REQUIRED)
    target_link_libraries(ned_editor_layer PUBLIC ZLIB::ZLIB)
  endif()
  target_link_libraries(ned_editor_layer PUBLIC
    winhttp
    secur32
    crypt32
    rpcrt4
    ws2_32
    advapi32
  )
endif()
target_compile_features(ned_editor_layer PUBLIC cxx_std_20)
ned_apply_feature_defs(ned_editor_layer)
if(NED_ENABLE_GIT)
  add_dependencies(ned_editor_layer libgit2 util llhttp xdiff)
endif()

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
set(NED_TEST_SOURCES
  main.cpp
  # Monaco / model
  editor/text_model_data_test.cpp
  editor/text_model_test.cpp
  editor/model_edit_operation_test.cpp
  editor/editable_text_model_test.cpp
  editor/editor_operations_test.cpp
  editor/text_buffer_test.cpp
  # Layer
  editor/editor_commands_test.cpp
  editor/multi_cursor_test.cpp
  editor/undo_service_test.cpp
  editor/save_service_test.cpp
  editor/utf8_utils_test.cpp
  editor/capture_map_test.cpp
  editor/highlight_queries_test.cpp
)
if(NED_ENABLE_GIT)
  list(APPEND NED_TEST_SOURCES editor/git_libgit2_test.cpp)
endif()

add_executable(ned_tests ${NED_TEST_SOURCES})

target_include_directories(ned_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/editor
  ${CMAKE_SOURCE_DIR}/lib/imgui
  ${CMAKE_SOURCE_DIR}/lib/treesitter/tree-sitter/lib/include
)
if(NED_ENABLE_GIT)
  target_include_directories(ned_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/libgit2/include
  )
endif()
if(GLFW_INCLUDE_DIR)
  target_include_directories(ned_tests PRIVATE ${GLFW_INCLUDE_DIR})
endif()

# Query compile tests resolve scm files under the source tree.
target_compile_definitions(ned_tests PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Monaco tests need text_model + state/ops from layer (avoid duplicate symbols).
target_sources(ned_tests PRIVATE monaco/text_model.cpp)
target_link_libraries(ned_tests PRIVATE
  ned_editor_layer
  tree-sitter-lib
  ${TS_GRAMMAR_LIBS}
)
target_compile_features(ned_tests PRIVATE cxx_std_20)
ned_apply_feature_defs(ned_tests)

include(CTest)
add_test(NAME ned_tests COMMAND ned_tests)
set_tests_properties(ned_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
