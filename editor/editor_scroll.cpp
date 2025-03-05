#include "editor_scroll.h"
#include "editor.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Global instance
EditorScroll gEditorScroll;

EditorScroll::EditorScroll() : scrollPos(0, 0), scrollX(0.0f), ensureCursorVisibleFrames(0), requestedScrollX(0), requestedScrollY(0), hasScrollRequest(false), pendingBookmarkScroll(false), pendingScrollX(0.0f), pendingScrollY(0.0f) {}

// Private helper that doesn't reference Editor directly to avoid circular dependency
int EditorScroll::getLineFromPosition(const std::vector<int> &line_starts, int pos)
{
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), pos);
    return std::distance(line_starts.begin(), it) - 1;
}

void EditorScroll::updateScrollAnimation(EditorState &state, float &current_scroll_x, float &current_scroll_y, float dt)
{
    // Animation should complete in ~0.1 seconds
    const float animation_speed = 15.0f; // Move 15% of remaining distance per frame
    const float min_step = 1.0f;         // Minimum pixels to move per frame
    const float threshold = 0.5f;        // Snap threshold

    // Handle horizontal animation
    if (scrollAnimation.active_x) {
        float target_x = scrollAnimation.target_x;
        float delta_x = target_x - current_scroll_x;

        // If we're close enough, snap to target
        if (std::abs(delta_x) < threshold) {
            current_scroll_x = target_x;
            scrollAnimation.active_x = false;
        } else {
            // Move a percentage of the remaining distance
            float step = delta_x * animation_speed * dt;

            // Ensure minimum step size for consistent motion
            if (std::abs(step) < min_step) {
                step = min_step * (delta_x > 0 ? 1.0f : -1.0f);
            }

            // If we would overshoot, just snap to target
            if ((delta_x > 0 && step > delta_x) || (delta_x < 0 && step < delta_x)) {
                current_scroll_x = target_x;
                scrollAnimation.active_x = false;
            } else {
                current_scroll_x += step;
            }
        }
    }

    // Handle vertical animation
    if (scrollAnimation.active_y) {
        float target_y = scrollAnimation.target_y;
        float delta_y = target_y - current_scroll_y;

        // If we're close enough, snap to target
        if (std::abs(delta_y) < threshold) {
            current_scroll_y = target_y;
            scrollAnimation.active_y = false;
        } else {
            // Move a percentage of the remaining distance
            float step = delta_y * animation_speed * dt;

            // Ensure minimum step size for consistent motion
            if (std::abs(step) < min_step) {
                step = min_step * (delta_y > 0 ? 1.0f : -1.0f);
            }
            // If we would overshoot, just snap to target
            if ((delta_y > 0 && step > delta_y) || (delta_y < 0 && step < delta_y)) {
                current_scroll_y = target_y;
                scrollAnimation.active_y = false;
            } else {
                current_scroll_y += step;
            }
        }
    }

    // Store updated scroll positions
    scrollX = current_scroll_x;
    scrollPos.y = current_scroll_y;
}

void EditorScroll::processMouseWheelForEditor(float line_height, float &current_scroll_y, float &current_scroll_x, EditorState &editor_state)
{
    if (ImGui::IsWindowHovered() && !editor_state.block_input) {
        float wheel_y = ImGui::GetIO().MouseWheel;
        float wheel_x = ImGui::GetIO().MouseWheelH;

        // Reduce the multiplier from 3 to 1.5 for a slower vertical scroll
        if (wheel_y != 0) {
            current_scroll_y -= wheel_y * line_height * 1.0f;
            current_scroll_y = std::max(0.0f, std::min(current_scroll_y, ImGui::GetScrollMaxY()));
            scrollPos.y = current_scroll_y;
        }

        // Also reduce the horizontal scroll speed multiplier
        if (wheel_x != 0) {
            current_scroll_x -= wheel_x * ImGui::GetFontSize() * 1.0f;
            current_scroll_x = std::max(0.0f, std::min(current_scroll_x, ImGui::GetScrollMaxX()));
            scrollX = current_scroll_x;
        }
    }
}

void EditorScroll::processMouseWheelScrolling(float line_height, EditorState &state)
{
    float wheel_y = ImGui::GetIO().MouseWheel;
    float wheel_x = ImGui::GetIO().MouseWheelH;
    if (wheel_y != 0 || wheel_x != 0) {
        if (ImGui::GetIO().KeyShift) { // Horizontal scrolling with Shift+Scroll
            float scroll_amount = wheel_y * ImGui::GetFontSize() * 1;
            float new_scroll_x = ImGui::GetScrollX() - scroll_amount;
            new_scroll_x = std::max(0.0f, std::min(new_scroll_x, ImGui::GetScrollMaxX()));
            ImGui::SetScrollX(new_scroll_x);
            scrollX = new_scroll_x;
        } else { // Vertical scrolling
            float new_scroll_y = ImGui::GetScrollY() - wheel_y * line_height * 3;
            new_scroll_y = std::max(0.0f, std::min(new_scroll_y, ImGui::GetScrollMaxY()));
            ImGui::SetScrollY(new_scroll_y);
            scrollPos.y = new_scroll_y;
        }
    }
}

float EditorScroll::calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos)
{
    float x = text_pos.x;
    for (int i = 0; i < cursor_pos; i++) {
        if (text[i] == '\n') {
            x = text_pos.x;
        } else {
            x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
        }
    }
    return x;
}

ScrollChange EditorScroll::ensureCursorVisible(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width)
{
    // Get current scroll offsets
    float scroll_x = ImGui::GetScrollX();
    float scroll_y = ImGui::GetScrollY();

    // Calculate viewport dimensions
    float scrollbar_width = ImGui::GetStyle().ScrollbarSize;
    float additional_padding = 80.0f;
    float viewport_width = window_width - scrollbar_width - additional_padding;
    float viewport_height = window_height;

    // Calculate cursor position
    float abs_cursor_x = calculateCursorXPosition(text_pos, text, state.cursor_column);
    // Use our own getLineFromPosition to avoid circular dependency
    int cursor_line = getLineFromPosition(state.editor_content_lines, state.cursor_column);
    float abs_cursor_y = text_pos.y + cursor_line * line_height;

    // Calculate cursor position relative to viewport
    float visible_cursor_x = abs_cursor_x - text_pos.x - scroll_x;
    float visible_cursor_y = abs_cursor_y - text_pos.y - scroll_y;

    // Margins - space to keep between cursor and edges
    float margin_x = ImGui::GetFontSize() * 3.0f;
    float margin_y = line_height * 1.5f;

    // Calculate distances from each edge (negative means cursor is outside)
    float dist_left = visible_cursor_x;
    float dist_right = viewport_width - visible_cursor_x;
    float dist_top = visible_cursor_y;
    float dist_bottom = viewport_height - visible_cursor_y - line_height;

    bool scroll_x_changed = false;
    bool scroll_y_changed = false;
    float new_scroll_x = scroll_x;
    float new_scroll_y = scroll_y;

    // Check if cursor needs horizontal scrolling
    if (dist_left < margin_x) {
        // Cursor too close to or beyond left edge
        new_scroll_x = scroll_x - (margin_x - dist_left);
        new_scroll_x = std::max(0.0f, new_scroll_x);
        scroll_x_changed = true;
    } else if (dist_right < margin_x) {
        // Cursor too close to or beyond right edge
        new_scroll_x = scroll_x + (margin_x - dist_right);
        new_scroll_x = std::min(new_scroll_x, ImGui::GetScrollMaxX());
        scroll_x_changed = true;
    }

    // Check if cursor needs vertical scrolling
    if (dist_top < margin_y) {
        // Cursor too close to or beyond top edge
        new_scroll_y = scroll_y - (margin_y - dist_top);
        new_scroll_y = std::max(0.0f, new_scroll_y);
        scroll_y_changed = true;
    } else if (dist_bottom < margin_y) {
        // Cursor too close to or beyond bottom edge
        new_scroll_y = scroll_y + (margin_y - dist_bottom);
        new_scroll_y = std::min(new_scroll_y, ImGui::GetScrollMaxY());
        scroll_y_changed = true;
    }

    // Store target positions in our scroll state variables
    if (scroll_x_changed) {
        scrollX = new_scroll_x;
    }

    if (scroll_y_changed) {
        scrollPos.y = new_scroll_y;
    }

    return {scroll_y_changed, scroll_x_changed};
}

void EditorScroll::adjustScrollForCursorVisibility(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width, float &current_scroll_y, float &current_scroll_x, CursorVisibility &ensure_cursor_visible)
{
    // IMPORTANT: Update scroll positions from ImGui to capture manual scrolling
    current_scroll_y = ImGui::GetScrollY();
    current_scroll_x = ImGui::GetScrollX();

    // Update our internal state
    scrollPos.y = current_scroll_y;
    scrollX = current_scroll_x;

    // First check if there's a direct scroll request
    float requested_x, requested_y;
    if (handleScrollRequest(requested_x, requested_y)) {
        // Set animation targets for direct requests
        scrollAnimation.active_x = true;
        scrollAnimation.target_x = requested_x;
        scrollAnimation.active_y = true;
        scrollAnimation.target_y = requested_y;

        // Store the targets in state variables too
        scrollX = requested_x;
        scrollPos.y = requested_y;

        // Reset visibility flags
        ensure_cursor_visible.vertical = false;
        ensure_cursor_visible.horizontal = false;

        return;
    }

    // Only try to ensure cursor visibility if not manually scrolled too far
    if (ensure_cursor_visible.vertical || ensure_cursor_visible.horizontal) {
        // Call ensureCursorVisible to calculate scroll adjustments
        ScrollChange scroll_change = ensureCursorVisible(text_pos, text, state, line_height, window_height, window_width);

        // If scroll changes are needed, set animation targets
        if (scroll_change.horizontal) {
            scrollAnimation.active_x = true;
            scrollAnimation.target_x = scrollX;
        }

        if (scroll_change.vertical) {
            scrollAnimation.active_y = true;
            scrollAnimation.target_y = scrollPos.y;
        }

        // Reset the visibility flags
        ensure_cursor_visible.vertical = false;
        ensure_cursor_visible.horizontal = false;
    }
}

void EditorScroll::handleCursorMovementScroll(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width)
{
    // Get current viewport bounds
    float visible_start_y = ImGui::GetScrollY();
    float visible_end_y = visible_start_y + window_height;
    float visible_start_x = ImGui::GetScrollX();
    float visible_end_x = visible_start_x + window_width;

    // Get cursor position
    // Use our own method for getting line from position
    int cursor_line = getLineFromPosition(state.editor_content_lines, state.cursor_column);
    float cursor_y = text_pos.y + (cursor_line * line_height);
    float cursor_x = calculateCursorXPosition(text_pos, text, state.cursor_column);

    // Handle vertical scrolling
    if (cursor_y < visible_start_y) {
        scrollPos.y = cursor_y - text_pos.y;
    } else if (cursor_y + line_height > visible_end_y) {
        scrollPos.y = cursor_y + line_height - window_height;
    }

    // Handle horizontal scrolling
    if (cursor_x < visible_start_x) {
        scrollX = cursor_x - text_pos.x;
    } else if (cursor_x > visible_end_x) {
        scrollX = cursor_x - window_width + ImGui::GetFontSize();
    }
}

void EditorScroll::scrollToLine(EditorState &state, int lineNumber, float line_height)
{
    if (lineNumber < 0 || lineNumber >= state.editor_content_lines.size()) {
        return; // Invalid line number
    }

    // Calculate the Y position for this line
    float lineY = lineNumber * line_height;

    // Use requestScroll to trigger smooth scroll animation
    requestScroll(scrollX, lineY);
}

void EditorScroll::scrollToCharacter(EditorState &state, int charIndex, const ImVec2 &text_pos, const std::string &text, float line_height)
{
    if (charIndex < 0 || charIndex >= text.size()) {
        return; // Invalid character index
    }

    // Find the line for this character
    int line = getLineFromPosition(state.editor_content_lines, charIndex);

    // Calculate position
    float charX = calculateCursorXPosition(text_pos, text, charIndex);
    float lineY = text_pos.y + line * line_height - text_pos.y; // Relative to top

    // Use requestScroll to trigger smooth scroll animation
    requestScroll(charX - text_pos.x, lineY);
}

void EditorScroll::centerOnCursor(EditorState &state, const ImVec2 &text_pos, const std::string &text, float line_height, float window_height, float window_width)
{
    int cursor_line = getLineFromPosition(state.editor_content_lines, state.cursor_column);

    // Calculate the cursor position
    float cursor_x = calculateCursorXPosition(text_pos, text, state.cursor_column) - text_pos.x;
    float cursor_y = cursor_line * line_height;

    // Calculate centered scroll positions
    float centered_x = cursor_x - window_width / 2;
    float centered_y = cursor_y - window_height / 2;

    // Ensure we don't scroll out of bounds
    centered_x = std::max(0.0f, std::min(centered_x, ImGui::GetScrollMaxX()));
    centered_y = std::max(0.0f, std::min(centered_y, ImGui::GetScrollMaxY()));

    // Request smooth scroll to center position
    requestScroll(centered_x, centered_y);
}
