#include "../files/files.h"
#include "editor.h"
#include "editor_tree_sitter.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <tree_sitter/api.h>

EditorHighlight gEditorHighlight;

EditorHighlight::EditorHighlight()
	: highlightingInProgress(false), cancelHighlightFlag(false)
{
}

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
	TreeSitter::clearQueryCache();
	TreeSitter::colorsNeedUpdate = true;
	if (!gFileExplorer.currentFile.empty())
	{
		highlightContent();
	}
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

void EditorHighlight::highlightContent(bool fullRehighlight, bool sync)
{
	std::lock_guard<std::mutex> lock(highlight_mutex);

	if (!sync)
	{
		cancelHighlighting();
	}

	TreeSitter::updateThemeColors();

	std::string content_copy;
	std::vector<ImVec4> colors_param_copy;
	std::string currentFile_copy;
	std::string extension_copy;

	{
		std::lock_guard<std::mutex> state_lock(editor_state.colorsMutex);

		if (editor_state.fileContent.empty())
		{
			editor_state.fileColors.clear();
			highlightingInProgress = false;
			return;
		}

		// Handle large files quickly (bypasses async task)
		const size_t LARGE_FILE_THRESHOLD = 1 * 1024 * 1024 * 100; // 100MB
		if (editor_state.fileContent.size() > LARGE_FILE_THRESHOLD)
		{
			editor_state.fileColors.assign(editor_state.fileContent.size(),
										   TreeSitter::cachedColors.text);
			highlightingInProgress = false; // Not strictly necessary here
			return;
		}

		if (!validateHighlightContentParams())
		{
			return;
		}

		content_copy = editor_state.fileContent;
		colors_param_copy =
			editor_state.fileColors; // Copied while editor_state is locked
		currentFile_copy = gFileExplorer.currentFile;
		extension_copy = fs::path(currentFile_copy).extension().string();
	} // editor_state.colorsMutex is released

	// Define the highlighting logic as a lambda that can be reused
	auto performHighlighting = [this, content_copy, extension_copy, fullRehighlight](
								   std::vector<ImVec4> &colors) {
		try
		{
			colors.assign(content_copy.size(), TreeSitter::cachedColors.text);

			if (gSettings.getTreesitterMode())
			{
				TreeSitter::parse(content_copy, colors, extension_copy, fullRehighlight);
			} else // Custom lexers or fallback for unsupported extensions
			{
				if (extension_copy == ".cpp" || extension_copy == ".h" ||
					extension_copy == ".hpp")
				{
					cppLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".py")
				{
					pythonLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".html" || extension_copy == ".cshtml")
				{
					htmlLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".js" || extension_copy == ".jsx")
				{
					jsxLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".tsx" || extension_copy == ".ts")
				{
					tsxLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".java")
				{
					javaLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".cs")
				{
					csharpLexer.applyHighlighting(content_copy, colors, 0);
				} else if (extension_copy == ".css")
				{
					cssLexer.applyHighlighting(content_copy, colors, 0);
				}
			}
		} catch (const std::exception &e)
		{
			std::cerr << "Highlighting error: " << e.what() << std::endl;
			colors.assign(content_copy.size(), TreeSitter::cachedColors.text);
		}
	};

	if (sync)
	{
		// Synchronous highlighting - perform immediately
		std::lock_guard<std::mutex> state_lock(editor_state.colorsMutex);
		performHighlighting(editor_state.fileColors);
	} else
	{
		// Asynchronous highlighting - use existing async logic
		highlightingInProgress = true;

		highlightFuture = std::async(
			std::launch::async,
			[this,
			 content_copy,
			 colors_param_copy,
			 currentFile_copy,
			 performHighlighting]() mutable {
				std::vector<ImVec4> current_colors = colors_param_copy;

				if (cancelHighlightFlag.load())
				{
					highlightingInProgress = false;
					return;
				}

				performHighlighting(current_colors);

				std::lock_guard<std::mutex> state_lock(editor_state.colorsMutex);

				if (!cancelHighlightFlag.load() &&
					currentFile_copy == gFileExplorer.currentFile &&
					content_copy == editor_state.fileContent)
				{
					editor_state.fileColors = current_colors;
				}
				highlightingInProgress = false;
			});
	}
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
