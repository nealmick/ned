# ned unit tests
#   - Monaco-shaped document model suite (EditorState + EditorOperations)
#   - Layer suite: commands, undo, save, git, UTF-8 helpers
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
  ${CMAKE_SOURCE_DIR}/editor/services/git/git_repo.cpp
  ${CMAKE_SOURCE_DIR}/editor/services/git/line_diff.cpp
  # ImGui core + FreeType (matches app IMGUI_USER_CONFIG); no GLFW backend.
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/lib/imgui/misc/freetype/imgui_freetype.cpp
)

add_library(ned_editor_layer STATIC ${NED_LAYER_SOURCES})
target_include_directories(ned_editor_layer PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/editor
  ${CMAKE_SOURCE_DIR}/lib/imgui
  ${CMAKE_SOURCE_DIR}/lib/imgui/misc/freetype
  ${CMAKE_SOURCE_DIR}/lib/libgit2/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)
# editor_utils.h pulls GLFW for rainbow helper; keep include path available.
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
  PATHS /usr/local/include /opt/homebrew/include ${GLFW3_INCLUDE_DIRS}
)
if(GLFW_INCLUDE_DIR)
  target_include_directories(ned_editor_layer PUBLIC ${GLFW_INCLUDE_DIR})
endif()
find_package(Freetype REQUIRED)
# libgit2 static needs its internal deps (same as main ned target).
target_link_libraries(ned_editor_layer PUBLIC
  libgit2
  util
  llhttp
  xdiff
  pcre
  Freetype::Freetype
  $<$<PLATFORM_ID:Darwin>:z>
  $<$<PLATFORM_ID:Darwin>:iconv>
  $<$<PLATFORM_ID:Linux>:z>
  $<$<PLATFORM_ID:Linux>:pthread>
)
if(APPLE)
  target_link_libraries(ned_editor_layer PUBLIC
    "-framework Security"
    "-framework GSS"
    "-framework CoreFoundation"
  )
endif()
target_compile_features(ned_editor_layer PUBLIC cxx_std_20)
add_dependencies(ned_editor_layer libgit2 util llhttp xdiff)

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
add_executable(ned_tests
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
  editor/git_libgit2_test.cpp
  editor/utf8_utils_test.cpp
)

target_include_directories(ned_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/editor
  ${CMAKE_SOURCE_DIR}/lib/imgui
  ${CMAKE_SOURCE_DIR}/lib/libgit2/include
)
if(GLFW_INCLUDE_DIR)
  target_include_directories(ned_tests PRIVATE ${GLFW_INCLUDE_DIR})
endif()

# Monaco tests need text_model + state/ops from layer (avoid duplicate symbols).
target_sources(ned_tests PRIVATE monaco/text_model.cpp)
target_link_libraries(ned_tests PRIVATE ned_editor_layer)
target_compile_features(ned_tests PRIVATE cxx_std_20)

include(CTest)
add_test(NAME ned_tests COMMAND ned_tests)
set_tests_properties(ned_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
