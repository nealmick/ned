#include "editor_highlight.h"
#include "../files/files.h"
#include "editor.h"
#include "editor_tree_sitter.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <tree_sitter/api.h>

// Global instance
EditorHighlight gEditorHighlight;

EditorHighlight::EditorHighlight() : highlightingInProgress(false), cancelHighlightFlag(false) {}

void EditorHighlight::cancelHighlighting()
{
	cancelHighlightFlag = true;
	if (highlightFuture.valid())
	{
		highlightFuture.wait();
	}
	cancelHighlightFlag = false;
}

void EditorHighlight::forceColorUpdate()
{
	pythonLexer.forceColorUpdate();
	cppLexer.forceColorUpdate();
	htmlLexer.forceColorUpdate();
	jsxLexer.forceColorUpdate();
	tsxLexer.forceColorUpdate();

	TreeSitter::refreshColors();
}

bool EditorHighlight::validateHighlightContentParams()
{
	if (editor_state.fileContent.empty())
	{
		std::cerr << "Error: Empty content in highlightContent" << std::endl;
		return false;
	}
	if (editor_state.fileColors.empty())
	{
		std::cerr << "Error: Empty colors vector in highlightContent" << std::endl;
		return false;
	}
	if (editor_state.fileContent.size() != editor_state.fileColors.size())
	{
		std::cerr << "Error: Mismatch between content and colors size "
					 "in highlightContent"
				  << std::endl;
		return false;
	}

	return true;
}

void EditorHighlight::highlightContent()
{
	std::lock_guard<std::mutex> lock(highlight_mutex);
	std::cout << "\033[36mEditorHighlight:\033[0m Highlight Content\n";

	cancelHighlighting();

	// Create safe copies of content and colors
	std::string content_copy;
	std::vector<ImVec4> colors_copy;
	std::string currentFile;
	std::string extension;

	{
		std::lock_guard<std::mutex> state_lock(editor_state.colorsMutex);

		// Handle large files quickly
		const size_t LARGE_FILE_THRESHOLD = 100 * 1024;
		if (editor_state.fileContent.size() > LARGE_FILE_THRESHOLD)
		{
			editor_state.fileColors.resize(editor_state.fileContent.size(),
										   TreeSitter::cachedColors.text);

			return;
		}

		if (!validateHighlightContentParams())
			return;

		// Create copies while locked
		content_copy = editor_state.fileContent;
		colors_copy = editor_state.fileColors;
		currentFile = gFileExplorer.currentFile;
		extension = fs::path(currentFile).extension().string();
	}

	highlightingInProgress = true;
	cancelHighlightFlag = false;

	highlightFuture =
		std::async(std::launch::async,
				   [this, content_copy, colors_copy, currentFile, extension]() mutable {
					   try
					   {
						   if (cancelHighlightFlag.load())
						   {
							   highlightingInProgress = false;
							   return;
						   }
						   if (useTreeSitter)
						   {
							   TreeSitter::parse(content_copy, colors_copy, extension);

						   } else
						   {
							   // use custom lexers....
							   if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
							   {
								   cppLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".py")
							   {
								   pythonLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".html" || extension == ".cshtml")
							   {
								   htmlLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".js" || extension == ".jsx")
							   {
								   jsxLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".tsx" || extension == ".ts")
							   {
								   tsxLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".java")
							   {
								   javaLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".cs")
							   {
								   csharpLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else if (extension == ".css")
							   {
								   cssLexer.applyHighlighting(content_copy, colors_copy, 0);
							   } else
							   {
								   // Set default color for entire content
								   std::fill(colors_copy.begin(),
											 colors_copy.end(),
											 ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
							   }
						   }

						   // Safely update colors if still valid
						   std::lock_guard<std::mutex> state_lock(editor_state.colorsMutex);

						   if (!cancelHighlightFlag && currentFile == gFileExplorer.currentFile &&
							   content_copy == editor_state.fileContent &&
							   colors_copy.size() == editor_state.fileColors.size())
						   {
							   editor_state.fileColors = colors_copy; // Copy, don't move
							   std::cout << "Applied new highlighting\n";
						   }
					   } catch (const std::exception &e)
					   {
						   std::cerr << "Highlighting error: " << e.what() << std::endl;
					   }
					   highlightingInProgress = false;
				   });
}

void EditorHighlight::setTheme(const std::string &themeName) { loadTheme(themeName); }

void EditorHighlight::loadTheme(const std::string &themeName)
{
	auto &settings = gSettings.getSettings();
	if (settings.contains("themes") && settings["themes"].contains(themeName))
	{
		auto &theme = settings["themes"][themeName];
		for (const auto &[key, value] : theme.items())
		{
			themeColors[key] = ImVec4(value[0], value[1], value[2], value[3]);
		}
	} else
	{
		// Set default colors if theme not found
		themeColors["keyword"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
		themeColors["function"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
		themeColors["string"] = ImVec4(0.87f, 0.87f, 0.0f, 1.0f);
		themeColors["number"] = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);
		themeColors["comment"] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	}
}
