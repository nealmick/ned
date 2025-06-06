/*
	File: files.cpp
	Description: Main file logic, handles rendering file tree, saving after
   changes, and more...
*/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nfd.h>
#include <sstream>

#include "../editor/editor_highlight.h"
#include "../editor/editor_line_jump.h"
#include "../lib/json.hpp"
#include "../util/close_popper.h"
#include "../util/icon_definitions.h"
#include "../util/settings.h"
#include "files.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <fstream>
using json = nlohmann::json;

#define NANOSVG_IMPLEMENTATION
#include "lib/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "lib/nanosvgrast.h"

const std::string UNDO_FILE = ".undo-redo-ned.json";

FileExplorer gFileExplorer;

void FileExplorer::loadIcons()
{
	for (const auto &iconFile : IconDefinitions::DEFAULT_ICONS)
	{
		loadSingleIcon(iconFile);
	}

	if (fileTypeIcons.empty())
	{
		createDefaultIcon();
	}
}

GLuint FileExplorer::createTexture(const unsigned char *pixels, int width, int height)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	return texture;
}

bool FileExplorer::loadSingleIcon(
	const std::string &iconFile) // iconFile is like "file.svg" or "folder.png"
{
	std::string primaryPath = "icons/" + iconFile;
	std::string finalPathToLoad;

	if (std::filesystem::exists(primaryPath))
	{
		finalPathToLoad = primaryPath;
		// std::cout << "[IconLoader] Found icon at primary path: " << finalPathToLoad << std::endl;
	} else
	{
#ifndef __APPLE__ // Only try the Debian package path if NOT on macOS
		std::string debian_package_icon_base_path = "/usr/share/Ned/icons/";
		std::string packagedPath = debian_package_icon_base_path + iconFile;

		if (std::filesystem::exists(packagedPath))
		{
			finalPathToLoad = packagedPath;
			// std::cout << "[IconLoader] Found icon at Debian package path: " << finalPathToLoad <<
			// std::endl;
		} else
		{
			// std::cerr << "[IconLoader] Icon not found. Tried:\n"
			//           << "  1. Relative/Dev path: " << primaryPath << "\n"
			//           << "  2. Debian package path: " << packagedPath << std::endl;
			return false; // Icon not found at either location on Linux
		}
#else
		// On macOS, if not found at primaryPath, it's just not found for this simple logic.
		// std::cerr << "[IconLoader] Icon not found at primary path (macOS): " << primaryPath <<
		// std::endl;
		return false; // Icon not found at primary path on macOS
#endif
	}

	// If finalPathToLoad is empty here, it means it wasn't found (should be caught by returns above)
	if (finalPathToLoad.empty())
	{
		// This case should ideally not be reached if the logic above is correct.
		// std::cerr << "[IconLoader] Critical error: finalPathToLoad is empty for icon: " <<
		// iconFile << std::endl;
		return false;
	}

	// Load SVG (or other image type if you adapt this part)
	// Assuming nsvgParseFromFile, IconDimensions, nsvgCreateRasterizer, etc. are available.
	NSVGimage *image = nsvgParseFromFile(finalPathToLoad.c_str(), "px", IconDimensions::SVG_DPI);
	if (!image)
	{
		std::cerr << "Error loading SVG file: " << finalPathToLoad << std::endl;
		return false;
	}

	// Create rasterizer
	NSVGrasterizer *rast = nsvgCreateRasterizer();
	if (!rast)
	{
		std::cerr << "Error creating SVG rasterizer" << std::endl;
		nsvgDelete(image);
		return false;
	}

	// Allocate pixel buffer
	// Ensure IconDimensions::WIDTH and IconDimensions::HEIGHT are correctly defined.
	auto pixels =
		std::make_unique<unsigned char[]>(IconDimensions::WIDTH * IconDimensions::HEIGHT * 4);

	// Rasterize SVG
	nsvgRasterize(rast,
				  image,
				  0,
				  0,
				  IconDimensions::WIDTH / image->width, // Or some other scaling factor
				  pixels.get(),
				  IconDimensions::WIDTH,
				  IconDimensions::HEIGHT,
				  IconDimensions::WIDTH * 4); // Stride

	// Create OpenGL texture
	// Ensure createTexture is correctly defined and returns a GLuint.
	GLuint texture = createTexture(pixels.get(), IconDimensions::WIDTH, IconDimensions::HEIGHT);
	if (texture == 0)
	{ // Assuming 0 indicates failure in createTexture
		std::cerr << "Error creating OpenGL texture for icon: " << finalPathToLoad << std::endl;
		nsvgDeleteRasterizer(rast);
		nsvgDelete(image);
		return false;
	}

	// Store in icon map
	std::string iconName = iconFile.substr(0, iconFile.find('.')); // Gets "file" from "file.svg"
	fileTypeIcons[iconName] = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));

	// Cleanup
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);

	return true;
}

void FileExplorer::createDefaultIcon()
{
	unsigned char defaultIcon[] = {
		255,
		255,
		255,
		255, // White pixel
		0,
		0,
		0,
		255 // Black pixel
	};

	GLuint texture = createTexture(defaultIcon, 2, 1);
	fileTypeIcons["default"] = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
}

void FileExplorer::openFolderDialog()
{
	std::cout << "\033[35mFiles:\033[0m Opening folder dialog" << std::endl;
	nfdchar_t *outPath = NULL;
	nfdresult_t result = NFD_PickFolder(NULL, &outPath);
	if (result == NFD_OKAY)
	{
		selectedFolder = outPath;
		std::cout << "\033[35mFiles:\033[0m Selected folder: " << outPath << std::endl;

		free(outPath);
		_showFileDialog = false;
		showWelcomeScreen = false;
		loadUndoRedoState();
	} else if (result == NFD_CANCEL)
	{
		std::cout << "\033[35mFiles:\033[0m User canceled folder selection." << std::endl;
		_showFileDialog = false; // Reset flag on cancel
	} else
	{
		std::cout << "\033[35mFiles:\033[0m Error: " << NFD_GetError() << std::endl;
	}
}

bool FileExplorer::readFileContent(const std::string &path)
{

	try
	{
		// First check if the path exists and is a regular file
		if (!std::filesystem::exists(path))
		{
			std::cout << "File does not exist: " << path << std::endl;
			return false;
		}

		if (!std::filesystem::is_regular_file(path))
		{
			std::cout << "Not a regular file: " << path << std::endl;
			return false;
		}

		// Now safely check file size
		std::error_code ec;
		uintmax_t fileSize = std::filesystem::file_size(path, ec);
		if (ec)
		{
			std::cout << "Error getting file size: " << ec.message() << std::endl;
			return false;
		}

		bool isTruncated = fileSize > MAX_FILE_SIZE;

		// Open file in binary mode
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			std::cout << "Failed to open file" << std::endl;
			return false;
		}

		// Read content (either full file or truncated)
		size_t readSize = isTruncated ? MAX_FILE_SIZE : fileSize;
		std::vector<char> buffer(readSize);
		file.read(buffer.data(), readSize);

		if (file.bad())
		{
			std::cout << "Error reading file content" << std::endl;
			return false;
		}

		std::string content(buffer.data(), file.gcount());

		// Check for binary content in the first part
		int nullCount = 0;
		size_t checkSize = std::min(content.length(), size_t(1024));

		for (size_t i = 0; i < checkSize; i++)
		{
			if (content[i] == 0 || (static_cast<unsigned char>(content[i]) < 32 &&
									content[i] != '\n' && content[i] != '\r' && content[i] != '\t'))
			{
				nullCount++;
			}
		}

		if (nullCount > checkSize / 10)
		{
			std::cout << "File appears to be binary" << std::endl;
			editor_state.fileContent = "Error: File appears to be binary and "
									   "cannot be displayed in editor.";
			return false;
		}

		// Add truncation notice if needed
		if (isTruncated)
		{
			std::string notice = "\n\n[File truncated - showing first " +
								 std::to_string(MAX_FILE_SIZE / (1024 * 1024)) + "MB of " +
								 std::to_string(fileSize / (1024 * 1024)) + "MB]\n";
			content += notice;
		}

		editor_state.fileContent = std::move(content);
		return true;
	} catch (const std::exception &e)
	{
		std::cout << "Error reading file: " << e.what() << std::endl;
		editor_state.fileContent = "Error: " + std::string(e.what());
		return false;
	}
}

void FileExplorer::updateFileColorBuffer()
{
	editor_state.fileColors.clear();
	editor_state.fileColors.resize(editor_state.fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

	if (editor_state.fileContent.size() != editor_state.fileColors.size())
	{
		std::cerr << "Error: Color buffer size mismatch" << std::endl;
		throw std::runtime_error("Color buffer size mismatch");
	}
}
void FileExplorer::updateFilePathStates(const std::string &path)
{
	currentFile = path;
	if (currentOpenFile != path)
	{
		previousOpenFile = currentOpenFile;
		currentOpenFile = path;
	}
	_unsavedChanges = false;
}

void FileExplorer::setupUndoManager(const std::string &path)
{
	auto it = fileUndoManagers.find(path);
	if (it == fileUndoManagers.end())
	{
		it = fileUndoManagers.emplace(path, UndoRedoManager()).first;
	}
	currentUndoManager = &(it->second);
	currentUndoManager->addState(editor_state.fileContent,
								 0,
								 editor_state.fileContent.size(),
								 editor_state.cursor_index);
}

void FileExplorer::handleLoadError()
{
	editor_state.fileContent = "Error: Unable to open file.";
	currentFile = "";
	editor_state.fileColors.clear();
	currentUndoManager = nullptr;
}

void FileExplorer::loadFileContent(const std::string &path, std::function<void()> afterLoadCallback)
{
	saveCurrentFile(); // Save current before loading new
	editor_state.cursor_index = 0;
	editor_state.ensure_cursor_visible.horizontal = true;
	editor_state.ensure_cursor_visible.vertical = true;

	try
	{
		// cancel any ongoing highlighting.,..
		gEditorHighlight.cancelHighlighting();

		if (!readFileContent(path))
		{
			handleLoadError();
			return;
		}

		_unsavedChanges = false;
		updateFilePathStates(path);
		updateFileColorBuffer();
		setupUndoManager(path);
		gEditorHighlight.highlightContent();

		notifyLSPFileOpen(path);

		if (afterLoadCallback)
		{
			afterLoadCallback();
		}

	} catch (const std::exception &e)
	{
		std::cerr << "Error loading file: " << e.what() << std::endl;
		handleLoadError();
	}
}

void FileExplorer::addUndoState(int changeStart, int changeEnd)
{
	if (currentUndoManager)
	{
		currentUndoManager->addState(editor_state.fileContent,
									 changeStart,
									 changeEnd,
									 editor_state.cursor_index);
		saveUndoRedoState();
	}
}
void FileExplorer::renderFileContent()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	gFileContentSearch.handleFindBoxActivation();

	gFileContentSearch.renderFindBox();

	bool text_changed;
	renderEditor(text_changed);

	// Process debounced undo states
	if (currentUndoManager)
	{
		currentUndoManager->update();
	}

	ImGui::PopStyleVar();
}

void FileExplorer::renderEditor(bool &text_changed)
{
	gEditor.textEditor();

	if (editor_state.text_changed && !editor_state.active_find_box)
	{
		_unsavedChanges = true;
	}
}
void FileExplorer::resetColorBuffer()
{
	// Get the current content size
	const size_t new_size = editor_state.fileContent.size();

	// Completely clear existing colors
	// editor_state.fileColors.clear();

	// Resize and fill ALL elements with white
	editor_state.fileColors.resize(new_size, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

	// Alternative: Use assign() for atomic operation
	// editor_state.fileColors.assign(new_size, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

	std::cout << "Reset " << new_size << " colors to white\n";
}
void FileExplorer::applyContentChange(const UndoRedoManager::State &state, bool preAllocate)
{
	gEditorHighlight.cancelHighlighting();

	if (preAllocate)
	{
		// Pre-allocate memory to prevent frequent reallocations (unchanged)
		editor_state.fileContent.reserve(
			std::max(editor_state.fileContent.capacity(), state.content.length() + 1024 * 1024));
		editor_state.fileColors.reserve(
			std::max(editor_state.fileColors.capacity(), state.content.length() + 1024 * 1024));
	}

	editor_state.fileContent = state.content;
	
	// Ensure cursor position is valid
	int cursor_pos = state.cursor_index;
	if (cursor_pos < 0) cursor_pos = 0;
	if (cursor_pos > static_cast<int>(state.content.length())) {
		cursor_pos = state.content.length();
	}
	editor_state.cursor_index = cursor_pos;
	
	// Reset selection state
	editor_state.selection_start = editor_state.selection_end = cursor_pos;
	editor_state.selection_active = false;
	editor_state.multi_selections.clear();
	
	// Reset color buffer and trigger highlighting
	resetColorBuffer();
	gEditorHighlight.highlightContent(true);

	_unsavedChanges = true;
}
void FileExplorer::handleUndo()
{
	if (currentUndoManager)
	{
		auto state = currentUndoManager->undo(editor_state.fileContent);
		applyContentChange(state);
		saveUndoRedoState();
	}
}

void FileExplorer::handleRedo()
{
	if (currentUndoManager)
	{
		auto state = currentUndoManager->redo(editor_state.fileContent);
		applyContentChange(state, true);
		saveUndoRedoState();
	}
}

// In files.cpp
void FileExplorer::saveUndoRedoState()
{
	if (selectedFolder.empty())
	{
		std::cerr << "Cannot save undo state - no project folder selected\n";
		return;
	}

	fs::path undoPath = fs::path(selectedFolder) / ".undo-redo-ned.json";

	json root;
	for (const auto &[path, manager] : fileUndoManagers)
	{
		root["files"][path] = manager.toJson();
	}

	std::ofstream file(undoPath);
	if (file)
	{
		file << root.dump(4);
	} else
	{
		std::cerr << "Failed to save undo/redo state to " << undoPath << "\n";
	}
}

void FileExplorer::loadUndoRedoState()
{
	if (selectedFolder.empty())
		return;

	fs::path undoPath = fs::path(selectedFolder) / ".undo-redo-ned.json";
	std::ifstream file(undoPath);
	if (!file)
		return;

	try
	{
		json root;
		file >> root;
		for (auto &[key, value] : root["files"].items())
		{
			fileUndoManagers[key].fromJson(value);
		}
	} catch (const std::exception &e)
	{
		std::cerr << "Error loading undo/redo state: " << e.what() << "\n";
	}
}

void FileExplorer::saveCurrentFile()
{
	if (!currentFile.empty() && _unsavedChanges)
	{
		// Check if we're dealing with a truncated file
		if (editor_state.fileContent.find("[File truncated - showing first") != std::string::npos)
		{
			std::cerr << "Cannot save truncated file content" << std::endl;
			// TODO: Show warning to user that they can't save changes to
			// truncated files
			return;
		}

		std::ofstream file(currentFile);
		if (file.is_open())
		{
			file << editor_state.fileContent;
			file.close();
			_unsavedChanges = false;
			// std::cout << "File saved: " << currentFile << std::endl;
			//  Track document version - start at 1 and increment on each save
			int &version = _documentVersions[currentFile];
			version = version == 0 ? 1 : version + 1;

			// Notify LSP after successful save
			gEditorLSP.didChange(currentFile, version);
		} else
		{
			std::cerr << "Unable to save file: " << currentFile << std::endl;
		}
	}
}

void FileExplorer::notifyLSPFileOpen(const std::string &filePath)
{
	gEditorLSP.didOpen(filePath, editor_state.fileContent);
}
