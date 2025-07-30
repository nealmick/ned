/*
   util/bookmarks.h
   This utility allows for saving a bookmark and restore back to the bookmark
   position. Each bookmark stores the current file, the cursor location, and
   scroll position. Up to 10 bookmarks can be set at once, to set a bookmark
   press cmd option 0-9. To restore a bookmark postion press cmd 0-9 with the
   bookmark you want to restore. This system allows for quick file and cursor
   position swapping without the need to keep multiple files open.
*/

#pragma once
#include "editor.h"
#include "editor_scroll.h"
#include "editor_types.h"

#include "imgui.h"
#include <array>
#include <string>

#include "../files/files.h"
#include "../util/keybinds.h"
#include "../util/terminal.h"

class Bookmarks
{
  private:
	struct Bookmark
	{
		std::string filePath;
		int cursorPosition;
		int lineNumber;
		float scrollX;
		float scrollY;
		bool isSet;

		// Default constructor
		Bookmark()
			: filePath(""), cursorPosition(0), lineNumber(0), scrollX(0), scrollY(0),
			  isSet(false)
		{
		}

		// Main constructor - updated to include scroll positions
		Bookmark(
			const std::string &path, int cursor, int line, float sx, float sy, bool set)
			: filePath(path), cursorPosition(cursor), lineNumber(line), scrollX(sx),
			  scrollY(sy), isSet(set)
		{
		}
	};

	static constexpr size_t NUM_BOOKMARKS = 9;
	std::array<Bookmark, NUM_BOOKMARKS> bookmarks;

  public:
	bool showBookmarksWindow = false;

	inline void setBookmark(size_t slot,
							const std::string &filePath,
							int cursorPosition,
							int lineNumber)
	{
		if (slot < NUM_BOOKMARKS)
		{
			float scrollX = ImGui::GetScrollX();
			float scrollY = ImGui::GetScrollY();
			bookmarks[slot] =
				Bookmark(filePath, cursorPosition, lineNumber, scrollX, scrollY, true);

			// Debug print
			std::cout << "Set bookmark " << slot
					  << " with scroll positions - X: " << scrollX << ", Y: " << scrollY
					  << std::endl;
		}
	}
	inline bool
	jumpToBookmark(size_t slot, FileExplorer &fileExplorer, EditorState &editorState)
	{
		if (slot < NUM_BOOKMARKS && bookmarks[slot].isSet)
		{
			if (bookmarks[slot].filePath != fileExplorer.currentFile)
			{
				// Create callback to set scroll after file loads
				auto setScroll = [this, slot]() {
					gEditorScroll.requestScroll(bookmarks[slot].scrollX,
												bookmarks[slot].scrollY);
				};

				fileExplorer.loadFileContent(bookmarks[slot].filePath, setScroll);
			} else
			{
				// Same file, request scroll immediately
				gEditorScroll.requestScroll(bookmarks[slot].scrollX,
											bookmarks[slot].scrollY);
			}

			editorState.cursor_index =
				bookmarks[slot].cursorPosition; // use content string index...
			// editorState.cursor_row = bookmarks[slot].lineNumber;
			gEditorScroll.setEnsureCursorVisibleFrames(
				-1); // Use EditorScroll method instead
			return true;
		}
		return false;
	}

	inline void handleBookmarkInput(FileExplorer &fileExplorer)
	{
		bool shift_pressed = ImGui::GetIO().KeyShift;
		bool alt_pressed = ImGui::GetIO().KeyAlt;
		bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;

		ImGuiKey toggleBookmarksKey = gKeybinds.getActionKey("toggle_bookmarks_menu");
		if (main_key && ImGui::IsKeyPressed(toggleBookmarksKey, false))
		{
			ClosePopper::closeAllExcept(ClosePopper::Type::Bookmarks); // RIGHT
			return;
		}
		const ImGuiKey numberKeys[9] = {ImGuiKey_1,
										ImGuiKey_2,
										ImGuiKey_3,
										ImGuiKey_4,
										ImGuiKey_5,
										ImGuiKey_6,
										ImGuiKey_7,
										ImGuiKey_8,
										ImGuiKey_9};

		for (size_t i = 0; i < NUM_BOOKMARKS; ++i)
		{
			if (main_key && ImGui::IsKeyPressed(numberKeys[i]))
			{
				if (shift_pressed || alt_pressed)
				{
					int lineNumber = EditorUtils::GetLineFromPosition(
						editor_state.editor_content_lines, editor_state.cursor_index);
					setBookmark(i,
								fileExplorer.currentFile,
								editor_state.cursor_index,
								lineNumber);
					std::cout << "Bookmark " << (i + 1) << " set at line " << lineNumber
							  << std::endl;
				} else
				{
					if (jumpToBookmark(i, fileExplorer, editor_state))
					{
						std::cout << "Jumped to bookmark " << (i + 1) << std::endl;
					} else
					{
						std::cout << "Bookmark " << (i + 1) << " not set" << std::endl;
					}
				}
			}
		}
	}

	inline void renderBookmarksWindow()
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 currentWindowPos = ImGui::GetWindowPos();
			ImVec2 currentWindowSize = ImGui::GetWindowSize();
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
				(mousePos.x < currentWindowPos.x ||
				 mousePos.x > currentWindowPos.x + currentWindowSize.x ||
				 mousePos.y < currentWindowPos.y ||
				 mousePos.y > currentWindowPos.y + currentWindowSize.y))
			{
				showBookmarksWindow = false;
				editor_state.block_input = false;
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showBookmarksWindow = false;
			editor_state.block_input = false;
		}
		bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;

		ImGuiKey toggleBookmarksKey = gKeybinds.getActionKey("toggle_bookmarks_menu");
		if (main_key && ImGui::IsKeyPressed(toggleBookmarksKey, false) &&
			!gTerminal.isTerminalVisible())
		{
			showBookmarksWindow = !showBookmarksWindow;
			if (!showBookmarksWindow)
			{
				editor_state.block_input = false;
			}
		}

		if (showBookmarksWindow)
		{

			editor_state.block_input = true;

			ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
			ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
										   ImGui::GetIO().DisplaySize.y * 0.35f),
									ImGuiCond_Always,
									ImVec2(0.5f, 0.5f));

			ImGuiWindowFlags windowFlags =
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse;

			// Push custom styles for the window
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
								10.0f); // Add rounded corners
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,
								1.0f); // Add border

			ImGui::PushStyleColor(
				ImGuiCol_FrameBg,
				ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
					   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
					   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
					   1.0f));
			ImGui::PushStyleColor(
				ImGuiCol_WindowBg,
				ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
					   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
					   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
					   1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

			ImGui::Begin("Bookmarks", nullptr, windowFlags);

			ImGui::TextUnformatted("Bookmarks");
			ImGui::Separator();

			ImGui::BeginChild("ScrollingRegion",
							  ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
							  false,
							  ImGuiWindowFlags_HorizontalScrollbar);
			for (size_t i = 0; i < NUM_BOOKMARKS; ++i)
			{
				if (bookmarks[i].isSet)
				{
					ImGui::Text("%zu:", i + 1);
					ImGui::SameLine();
					ImGui::TextWrapped("%s Line %d",
									   bookmarks[i].filePath.c_str(),
									   bookmarks[i].lineNumber - 1);
				} else
				{
					ImGui::Text(" %zu: Empty slot", i + 1);
				}
			}
			ImGui::EndChild();

			ImGui::Separator();
			ImGui::Text("Press Escape to close this window");

			ImGui::End();

			// Pop the styles we pushed earlier
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);
		}
	}
	inline bool isWindowOpen() const { return showBookmarksWindow; }
};

extern Bookmarks gBookmarks;
