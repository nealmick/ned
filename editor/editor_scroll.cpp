#include "editor_scroll.h"
#include "editor.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Global instance
EditorScroll gEditorScroll;

EditorScroll::EditorScroll() : requestedScrollX(0), requestedScrollY(0), hasScrollRequest(false) {}

void EditorScroll::updateScrollAnimation(EditorState &state, float &current_scroll_x, float &current_scroll_y, float dt)
{
    // Animation should complete in ~0.1 seconds
    const float animation_speed = 15.0f; // Move 15% of remaining distance per frame
    const float min_step = 1.0f;         // Minimum pixels to move per frame
    const float threshold = 0.5f;        // Snap threshold

    // Handle horizontal animation
    if (state.scroll_animation.active_x) {
        float target_x = state.scroll_animation.target_x;
        float delta_x = target_x - current_scroll_x;

        // If we're close enough, snap to target
        if (std::abs(delta_x) < threshold) {
            current_scroll_x = target_x;
            state.scroll_animation.active_x = false;

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
                state.scroll_animation.active_x = false;

            } else {
                current_scroll_x += step;
            }
        }
    }

    // Handle vertical animation
    if (state.scroll_animation.active_y) {
        float target_y = state.scroll_animation.target_y;
        float delta_y = target_y - current_scroll_y;

        // If we're close enough, snap to target
        if (std::abs(delta_y) < threshold) {
            current_scroll_y = target_y;
            state.scroll_animation.active_y = false;
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
                state.scroll_animation.active_y = false;

            } else {
                current_scroll_y += step;
            }
        }
    }
}

void EditorScroll::processMouseWheelForEditor(float line_height, float &current_scroll_y, float &current_scroll_x, EditorState &editor_state)
{
    if (ImGui::IsWindowHovered() && !editor_state.blockInput) {
        float wheel_y = ImGui::GetIO().MouseWheel;
        float wheel_x = ImGui::GetIO().MouseWheelH;

        // Reduce the multiplier from 3 to 1.5 for a slower vertical scroll
        if (wheel_y != 0) {
            current_scroll_y -= wheel_y * line_height * 1.0f;
            current_scroll_y = std::max(0.0f, std::min(current_scroll_y, ImGui::GetScrollMaxY()));
        }

        // Also reduce the horizontal scroll speed multiplier
        if (wheel_x != 0) {
            current_scroll_x -= wheel_x * ImGui::GetFontSize() * 1.0f;
            current_scroll_x = std::max(0.0f, std::min(current_scroll_x, ImGui::GetScrollMaxX()));
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
            state.scroll_pos.x = new_scroll_x;
        } else { // Vertical scrolling
            float new_scroll_y = ImGui::GetScrollY() - wheel_y * line_height * 3;
            new_scroll_y = std::max(0.0f, std::min(new_scroll_y, ImGui::GetScrollMaxY()));
            ImGui::SetScrollY(new_scroll_y);
            state.scroll_pos.y = new_scroll_y;
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
    float abs_cursor_x = calculateCursorXPosition(text_pos, text, state.cursor_pos);
    int cursor_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
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

    // Store target positions in state variables without directly applying scroll
    if (scroll_x_changed) {
        state.scroll_x = new_scroll_x;
    }

    if (scroll_y_changed) {
        state.scroll_pos.y = new_scroll_y;
    }

    return {scroll_y_changed, scroll_x_changed};
}

void EditorScroll::adjustScrollForCursorVisibility(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width, float &current_scroll_y, float &current_scroll_x, CursorVisibility &ensure_cursor_visible)
{
    // IMPORTANT: Update scroll positions from ImGui to capture manual scrolling
    current_scroll_y = ImGui::GetScrollY();
    current_scroll_x = ImGui::GetScrollX();
    state.scroll_pos.y = current_scroll_y;
    state.scroll_x = current_scroll_x;

    // First check if there's a direct scroll request
    float requested_x, requested_y;
    if (handleScrollRequest(requested_x, requested_y)) {
        // Set animation targets for direct requests
        state.scroll_animation.active_x = true;
        state.scroll_animation.target_x = requested_x;
        state.scroll_animation.active_y = true;
        state.scroll_animation.target_y = requested_y;

        // Store the targets in state variables too
        state.scroll_x = requested_x;
        state.scroll_pos.y = requested_y;

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
            state.scroll_animation.active_x = true;
            state.scroll_animation.target_x = state.scroll_x;
        }

        if (scroll_change.vertical) {
            state.scroll_animation.active_y = true;
            state.scroll_animation.target_y = state.scroll_pos.y;
        }

        // Reset the visibility flags
        ensure_cursor_visible.vertical = false;
        ensure_cursor_visible.horizontal = false;
    }
}