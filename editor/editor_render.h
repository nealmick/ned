/*
    File: editor_render.h
    Description: Handles rendering for the editor, separating rendering logic from the main editor
   class.

    This class is responsible for all visual aspects of the editor, including:
    - Text rendering with syntax highlighting
    - Selection highlighting
    - UI elements like line numbers
    - Window and layout setup

    By separating rendering logic from the main Editor class, we achieve cleaner
    separation of concerns and make the codebase more maintainable.
*/

#pragma once
#include "editor_types.h"
#include "imgui.h"

#include <string>
#include <vector>

// Forward declarations
class EditorRender;
extern EditorRender gEditorRender;

class EditorRender
{
  public:
    //--------------------------------------------------------------------------
    // Main Render Functions
    //--------------------------------------------------------------------------

    /**
     * Renders the complete editor frame including text content, line numbers and UI elements
     *
     * @param text The text content to render
     * @param colors Color information for syntax highlighting
     * @param editor_state Current editor state
     * @param text_pos Position where text rendering starts
     * @param line_height Height of each line in pixels
     * @param line_numbers_pos Position where line numbers panel starts
     * @param line_number_width Width of line numbers panel
     * @param size Overall editor size
     * @param total_height Total height of all content
     * @param editor_top_margin Top margin of editor content
     */
    void renderEditorFrame(std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &text_pos, float line_height, ImVec2 &line_numbers_pos, float line_number_width, ImVec2 &size, float total_height, float editor_top_margin);

    /**
     * Renders the main editor content area (text, selection and cursor)
     *
     * @param text The text content to render
     * @param colors Color information for syntax highlighting
     * @param editor_state Current editor state
     * @param line_height Height of each line in pixels
     * @param text_pos Position where text rendering starts
     */
    void renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos);

    /**
     * Renders text with syntax highlighting and selection
     * Uses a character-by-character approach for precise rendering and selection highlighting
     *
     * @param drawList ImGui draw list for rendering
     * @param pos Starting position for text rendering
     * @param text Text content to render
     * @param colors Color information for syntax highlighting
     * @param state Current editor state
     * @param line_height Height of each line in pixels
     */
    void renderText(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height);

    //--------------------------------------------------------------------------
    // Editor Window Setup and Management
    //--------------------------------------------------------------------------

    /**
     * Validates that the colors vector matches the text length and resizes if needed
     *
     * @param text The text content
     * @param colors Color information to validate
     * @return True if resizing was necessary
     */
    bool validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors);

    /**
     * Sets up the editor window and initializes layout parameters
     *
     * @param label Unique identifier for the editor
     * @param size Will be filled with the calculated editor size
     * @param line_number_width Will be filled with calculated line number panel width
     * @param line_height Will be filled with calculated line height
     * @param editor_top_margin Will be filled with top margin value
     * @param text_left_margin Will be filled with left margin value
     */
    void setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin);

    /**
     * Renders the line numbers panel and returns its position
     *
     * @param line_number_width Width of the line numbers panel
     * @param editor_top_margin Top margin of the editor
     * @return Position of the line numbers panel
     */
    ImVec2 renderLineNumbersPanel(float line_number_width, float editor_top_margin);

    /**
     * Sets up the main editor child window for text editing
     *
     * @param label Unique identifier for the editor
     * @param remaining_width Width available for editor content
     * @param content_width Width of the content (may be larger than visible area)
     * @param content_height Height of the content (may be larger than visible area)
     * @param current_scroll_y Current vertical scroll position (will be updated)
     * @param current_scroll_x Current horizontal scroll position (will be updated)
     * @param text_pos Will be filled with the position where text should be rendered
     * @param editor_top_margin Top margin of the editor
     * @param text_left_margin Left margin of the text
     * @param editor_state Current editor state
     */
    void beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state);
};