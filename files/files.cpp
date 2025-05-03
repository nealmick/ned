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
#include "../util/close_popper.h"
#include "../util/icon_definitions.h"
#include "../util/settings.h"
#include "files.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#define NANOSVG_IMPLEMENTATION
#include "lib/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "lib/nanosvgrast.h"

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

bool FileExplorer::loadSingleIcon(const std::string &iconFile)
{
	std::string fullPath = "icons/" + iconFile;
	if (!std::filesystem::exists(fullPath))
	{
		return false;
	}

	// Load SVG
	NSVGimage *image = nsvgParseFromFile(fullPath.c_str(), "px", IconDimensions::SVG_DPI);
	if (!image)
	{
		std::cerr << "Error loading SVG file: " << fullPath << std::endl;
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
	auto pixels =
		std::make_unique<unsigned char[]>(IconDimensions::WIDTH * IconDimensions::HEIGHT * 4);

	// Rasterize SVG
	nsvgRasterize(rast,
				  image,
				  0,
				  0,
				  IconDimensions::WIDTH / image->width,
				  pixels.get(),
				  IconDimensions::WIDTH,
				  IconDimensions::HEIGHT,
				  IconDimensions::WIDTH * 4);

	// Create OpenGL texture
	GLuint texture = createTexture(pixels.get(), IconDimensions::WIDTH, IconDimensions::HEIGHT);

	// Store in icon map
	std::string iconName = iconFile.substr(0, iconFile.find('.'));
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

		// Notify LSP about the newly opened file
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
	}
}
void FileExplorer::renderFileContent()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	gFileContentSearch.handleFindBoxActivation();

	if (editor_state.active_find_box)
	{
		gFileContentSearch.renderFindBox();
	}

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
	editor_state.cursor_index = state.cursor_index;
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
	}
}

void FileExplorer::handleRedo()
{
	if (currentUndoManager)
	{
		auto state = currentUndoManager->redo(editor_state.fileContent);
		applyContentChange(state, true);
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
