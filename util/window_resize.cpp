/*
File: window_resize.cpp
Description: Window resize handling class implementation for NED text editor.
*/

#include "window_resize.h"
#include <iostream>

WindowResize::WindowResize() : window(nullptr) {}

WindowResize::~WindowResize() {}

void WindowResize::initialize(GLFWwindow *window) { this->window = window; }

void WindowResize::renderResizeHandles()
{
	const float resizeBorder = RESIZE_BORDER;
	ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
	ImVec2 windowSize = ImGui::GetMainViewport()->Size;

	// Right edge
	ImGui::PushID("resize_right");
	ImGui::SetCursorScreenPos(
		ImVec2(windowPos.x + windowSize.x - resizeBorder, windowPos.y));
	ImGui::InvisibleButton("##resize_right", ImVec2(resizeBorder, windowSize.y));
	bool hoverRight = ImGui::IsItemHovered();
	ImGui::PopID();

	// Bottom edge
	ImGui::PushID("resize_bottom");
	ImGui::SetCursorScreenPos(
		ImVec2(windowPos.x, windowPos.y + windowSize.y - resizeBorder));
	ImGui::InvisibleButton("##resize_bottom", ImVec2(windowSize.x, resizeBorder));
	bool hoverBottom = ImGui::IsItemHovered();
	ImGui::PopID();

	// Corner
	ImGui::PushID("resize_corner");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x + windowSize.x - resizeBorder,
									 windowPos.y + windowSize.y - resizeBorder));
	ImGui::InvisibleButton("##resize_corner", ImVec2(resizeBorder, resizeBorder));
	bool hoverCorner = ImGui::IsItemHovered();
	ImGui::PopID();

	// Update cursors based on hover state first
	if (hoverCorner || resizingCorner)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
	} else if (hoverRight || resizingRight)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	} else if (hoverBottom || resizingBottom)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}
}

void WindowResize::handleManualResizing()
{
	ImGuiIO &io = ImGui::GetIO();
	int currentScreenWidth, currentScreenHeight;
	glfwGetWindowSize(window, &currentScreenWidth, &currentScreenHeight);

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	const float resizeBorder = RESIZE_BORDER;
	// Hover detection based on current mouse position and current window size
	// This is primarily for initiating the resize operation.
	bool hoverRightEdge =
		mouseX >= (currentScreenWidth - resizeBorder) && mouseX < currentScreenWidth;
	bool hoverBottomEdge =
		mouseY >= (currentScreenHeight - resizeBorder) && mouseY < currentScreenHeight;
	bool hoverCorner =
		hoverRightEdge && hoverBottomEdge; // More specific corner detection

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (hoverCorner)
		{
			resizingCorner = true;
			resizingRight = true;
			resizingBottom = true;
		} else if (hoverRightEdge)
		{
			resizingCorner = false;
			resizingRight = true;
			resizingBottom = false;
		} else if (hoverBottomEdge)
		{
			resizingCorner = false;
			resizingRight = false;
			resizingBottom = true;
		} else
		{
			resizingCorner = false;
			resizingRight = false;
			resizingBottom = false;
		}

		if (resizingRight || resizingBottom)
		{
			dragStart = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
			initialWindowSize = ImVec2(static_cast<float>(currentScreenWidth),
									   static_cast<float>(currentScreenHeight));
		}
	}

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		if (resizingRight || resizingBottom)
		{
			int newWidth = static_cast<int>(initialWindowSize.x);
			int newHeight = static_cast<int>(initialWindowSize.y);

			if (resizingRight)
			{
				newWidth = static_cast<int>(initialWindowSize.x + (mouseX - dragStart.x));
			}

			if (resizingBottom)
			{
				float deltaY = static_cast<float>(mouseY - dragStart.y);
				newHeight = static_cast<int>(initialWindowSize.y + deltaY);
			}
			newWidth = std::max(newWidth, 100);
			newHeight = std::max(newHeight, 100);

			// Conditional break or further check if values are extreme
			if (newHeight > 10000 || newHeight < 0 || newWidth > 10000 || newWidth < 0)
			{
				std::cerr << "CRITICAL: Extreme resize values detected before "
							 "glfwSetWindowSize!"
						  << std::endl;
			}

			glfwSetWindowSize(window, newWidth, newHeight);
		}
	}
	// Handle drag end
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		resizingRight = false;
		resizingBottom = false;
		resizingCorner = false;
	}
}

float WindowResize::clamp(float value, float min, float max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}