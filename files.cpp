/*  
    files.cpp
    Main file logic, handles rendering file tree, saving after changes, and more...
*/

#define GL_SILENCE_DEPRECATION

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nfd.h>
#include <sstream>

#include "files.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "util/settings.h"
#include "util/line_jump.h"
#include "util/close_popper.h"
#include "util/icon_definitions.h"

#define NANOSVG_IMPLEMENTATION
#include "lib/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "lib/nanosvgrast.h"



FileExplorer gFileExplorer;

void FileExplorer::loadIcons() {
    for (const auto& iconFile : IconDefinitions::DEFAULT_ICONS) {
        loadSingleIcon(iconFile);
    }
    
    if (fileTypeIcons.empty()) {
        createDefaultIcon();
    }
}

ImTextureID FileExplorer::getIconForFile(const std::string& filename) {
    std::string extension = fs::path(filename).extension().string();
    if (!extension.empty() && extension[0] == '.') {
        extension = extension.substr(1);
    }
    
    auto it = fileTypeIcons.find(extension);
    return (it != fileTypeIcons.end()) ? it->second : fileTypeIcons["default"];
}

GLuint FileExplorer::createTexture(const unsigned char* pixels, int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,GL_UNSIGNED_BYTE, pixels);
    return texture;
}

bool FileExplorer::loadSingleIcon(const std::string& iconFile) {
    std::string fullPath = "icons/" + iconFile;
    if (!std::filesystem::exists(fullPath)) {
        return false;
    }

    // Load SVG
    NSVGimage* image = nsvgParseFromFile(fullPath.c_str(), "px", IconDimensions::SVG_DPI);
    if (!image) {
        std::cerr << "Error loading SVG file: " << fullPath << std::endl;
        return false;
    }

    // Create rasterizer
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        std::cerr << "Error creating SVG rasterizer" << std::endl;
        nsvgDelete(image);
        return false;
    }

    // Allocate pixel buffer
    auto pixels = std::make_unique<unsigned char[]>(
        IconDimensions::WIDTH * IconDimensions::HEIGHT * 4
    );

    // Rasterize SVG
    nsvgRasterize(rast, image, 0, 0, 
                 IconDimensions::WIDTH / image->width,
                 pixels.get(), 
                 IconDimensions::WIDTH, 
                 IconDimensions::HEIGHT,
                 IconDimensions::WIDTH * 4);

    // Create OpenGL texture
    GLuint texture = createTexture(pixels.get(), 
                                 IconDimensions::WIDTH, 
                                 IconDimensions::HEIGHT);

    // Store in icon map
    std::string iconName = iconFile.substr(0, iconFile.find('.'));
    fileTypeIcons[iconName] = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));

    // Cleanup
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    
    return true;
}

void FileExplorer::createDefaultIcon() {
    unsigned char defaultIcon[] = {
        255, 255, 255, 255, // White pixel
        0,   0,   0,   255  // Black pixel
    };
    
    GLuint texture = createTexture(defaultIcon, 2, 1);
    fileTypeIcons["default"] = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
}

void FileExplorer::preserveOpenStates(const FileNode &oldNode,
                                      FileNode &newNode) {
  for (auto &newChild : newNode.children) {
    auto it = std::find_if(oldNode.children.begin(), oldNode.children.end(),
                           [&newChild](const FileNode &oldChild) {
                             return oldChild.fullPath == newChild.fullPath;
                           });
    if (it != oldNode.children.end()) {
      newChild.isOpen = it->isOpen;
      if (newChild.isDirectory && newChild.isOpen) {
        preserveOpenStates(*it, newChild);
      }
    }
  }
}

void FileExplorer::refreshFileTree() {
    // Get the current time
    double currentTime = glfwGetTime();

    // Check if enough time has passed since the last refresh
    if (currentTime - lastFileTreeRefreshTime < FILE_TREE_REFRESH_INTERVAL) {
        return;
    }

    // Update the last refresh time
    lastFileTreeRefreshTime = currentTime;

    if (!selectedFolder.empty()) {
        // Store the old root node to preserve states
        FileNode oldRoot = rootNode;
        
        // Reset root node but preserve its open state
        rootNode.name = fs::path(selectedFolder).filename().string();
        rootNode.fullPath = selectedFolder;
        rootNode.isDirectory = true;
        rootNode.isOpen = true;  // Root should stay open
        
        // Build the new tree
        buildFileTree(selectedFolder, rootNode);
        
        // Restore open states from old tree
        preserveOpenStates(oldRoot, rootNode);
    }
}

void FileExplorer::buildFileTree(const fs::path &path, FileNode &node) {
    // Don't clear children if they already exist and the node is open
    if (!node.isOpen && !node.children.empty()) {
        return;
    }

    std::vector<FileNode> newChildren;
    try {
        for (const auto &entry : fs::directory_iterator(path)) {
            FileNode child;
            child.name = entry.path().filename().string();
            child.fullPath = entry.path().string();
            child.isDirectory = entry.is_directory();
            
            // Find existing child to preserve its state
            auto existingChild = std::find_if(node.children.begin(), node.children.end(),
                [&child](const FileNode &existing) {
                    return existing.fullPath == child.fullPath;
                });
            
            if (existingChild != node.children.end()) {
                // Preserve the existing child's state and children
                child.isOpen = existingChild->isOpen;
                child.children = std::move(existingChild->children);
            }
            
            // If it's a directory and it's open, build its tree
            if (child.isDirectory && child.isOpen) {
                buildFileTree(child.fullPath, child);
            }
            
            newChildren.push_back(std::move(child));
        }
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Error accessing directory " << path << ": " << e.what() << std::endl;
    }

    // Sort directories first, then files by name
    std::sort(newChildren.begin(), newChildren.end(),
          [](const FileNode &a, const FileNode &b) {
              if (a.isDirectory != b.isDirectory) {
                  return a.isDirectory > b.isDirectory;
              }
              return a.name < b.name;
          });

    node.children = std::move(newChildren);
}

FileExplorer::TreeDisplayMetrics FileExplorer::calculateDisplayMetrics() {
    TreeDisplayMetrics metrics;
    metrics.currentFontSize = gSettings.getFontSize();
    metrics.folderIconSize = metrics.currentFontSize * 0.8f;
    metrics.fileIconSize = metrics.currentFontSize * 1.2f;
    metrics.itemHeight = ImGui::GetFrameHeight();
    metrics.indentWidth = 28.0f;
    metrics.cursorPos = ImGui::GetCursorPos();
    return metrics;
}

void FileExplorer::pushTreeStyles() {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, TreeStyleSettings::FRAME_ROUNDING);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, TreeStyleSettings::FRAME_PADDING);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, TreeStyleSettings::ITEM_SPACING);
}

ImTextureID FileExplorer::getFolderIcon(bool isOpen) {
    ImTextureID icon = isOpen ? fileTypeIcons["folder-open"] : fileTypeIcons["folder"];
    if (!icon) {
        icon = fileTypeIcons["folder"];
    }
    return icon ? icon : fileTypeIcons["default"];
}

void FileExplorer::renderNodeText(const std::string& name, bool isCurrentFile) {
    if (!isCurrentFile) {
        ImGui::Text("%s", name.c_str());
        return;
    }

    if (gSettings.getRainbowMode()) {
        ImVec4 fileColor = GetRainbowColor(ImGui::GetTime() * 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, fileColor);
        ImGui::Text("%s", name.c_str());
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, TreeStyleSettings::INACTIVE_TEXT);
        ImGui::Text("%s", name.c_str());
        ImGui::PopStyleColor();
    }
}

void FileExplorer::displayDirectoryNode(const FileNode& node, const FileExplorer::TreeDisplayMetrics& metrics, int& depth) {
    ImVec2 iconSize(metrics.folderIconSize, metrics.folderIconSize);
    float verticalPadding = (metrics.itemHeight - iconSize.y) * 0.5f;

    // Setup folder button
    ImGui::PushStyleColor(ImGuiCol_Button, TreeStyleSettings::TRANSPARENT_BG);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, TreeStyleSettings::HOVER_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, 
        ImVec2(TreeStyleSettings::HORIZONTAL_PADDING, verticalPadding));

    bool isOpen = ImGui::Button(("##" + node.fullPath).c_str(),
        ImVec2(ImGui::GetContentRegionAvail().x, metrics.itemHeight));
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Draw folder icon
    ImGui::SetCursorPos(ImVec2(
        metrics.cursorPos.x + depth * metrics.indentWidth + TreeStyleSettings::HORIZONTAL_PADDING,
        metrics.cursorPos.y + verticalPadding
    ));
    ImGui::Image(getFolderIcon(node.isOpen), iconSize);

    // Draw folder name
    ImGui::SameLine(depth * metrics.indentWidth + iconSize.x + 
        TreeStyleSettings::HORIZONTAL_PADDING + TreeStyleSettings::TEXT_PADDING);
    ImGui::SetCursorPosY(metrics.cursorPos.y + 
        (metrics.itemHeight - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::Text("%s", node.name.c_str());

    // Handle open/close and recursion
    if (isOpen) {
        const_cast<FileNode&>(node).isOpen = !node.isOpen;
        if (node.isOpen) {
            buildFileTree(node.fullPath, const_cast<FileNode&>(node));
        }
    }

    if (node.isOpen) {
        depth++;
        for (const auto& child : node.children) {
            displayFileTree(const_cast<FileNode&>(child));
        }
        depth--;
    }
}

void FileExplorer::displayFileNode(const FileNode& node, const TreeDisplayMetrics& metrics, int depth) {
    ImVec2 iconSize(metrics.fileIconSize, metrics.fileIconSize);
    ImTextureID fileIcon = getIconForFile(node.name);

    // Setup file button
    ImGui::PushStyleColor(ImGuiCol_Button, TreeStyleSettings::TRANSPARENT_BG);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, TreeStyleSettings::HOVER_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    bool clicked = ImGui::Button(("##" + node.fullPath).c_str(),
        ImVec2(ImGui::GetContentRegionAvail().x, metrics.itemHeight));

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Draw file icon
    ImGui::SetCursorPos(metrics.cursorPos);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * metrics.indentWidth + 
        TreeStyleSettings::LEFT_MARGIN);
    ImGui::Image(fileIcon, iconSize);

    // Draw filename
    ImGui::SameLine(depth * metrics.indentWidth + iconSize.x + 
        TreeStyleSettings::LEFT_MARGIN + 10);
    renderNodeText(node.name, node.fullPath == currentOpenFile);

    if (clicked) {
        loadFileContent(node.fullPath);
    }
}

void FileExplorer::displayFileTree(FileNode& node) {
    static int current_depth = 0;
    TreeDisplayMetrics metrics = calculateDisplayMetrics();

    pushTreeStyles();
    ImGui::PushID(node.fullPath.c_str());

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + current_depth * metrics.indentWidth);

    if (node.isDirectory) {
        displayDirectoryNode(node, metrics, current_depth);
    } else {
        displayFileNode(node, metrics, current_depth);
    }

    ImGui::PopID();
    ImGui::PopStyleVar(3);
}

void FileExplorer::openFolderDialog() {
    std::cout << "\033[35mFiles:\033[0m Opening folder dialog" << std::endl;
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_PickFolder(NULL, &outPath);
    if (result == NFD_OKAY) {
        selectedFolder = outPath;
        std::cout << "\033[35mFiles:\033[0m Selected folder: " << outPath << std::endl;
        free(outPath);
        _showFileDialog = false;
        setShowWelcomeScreen(false);  // Hide welcome screen when folder selected
    } else if (result == NFD_CANCEL) {
        std::cout << "\033[35mFiles:\033[0m User canceled folder selection." << std::endl;
        _showFileDialog = false;  // Reset flag on cancel
    } else {
        std::cout << "\033[35mFiles:\033[0m Error: " << NFD_GetError() << std::endl;
    }
}

void FileExplorer::refreshSyntaxHighlighting() {
  if (!currentFile.empty()) {
    std::string extension = fs::path(currentFile).extension().string();
    gEditor.setLanguage(extension);
    gEditor.highlightContent(fileContent, fileColors, 0, fileContent.size());
  }
}

void FileExplorer::resetEditorState() {
    editor_state.cursor_pos = 0;
    editor_state.current_line = 0;
    gEditor.cancelHighlighting();
}

void FileExplorer::updateFilePathStates(const std::string& path) {
    currentFile = path;
    if (currentOpenFile != path) {
        previousOpenFile = currentOpenFile;
        currentOpenFile = path;
    }
    _unsavedChanges = false;
}

bool FileExplorer::readFileContent(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    fileContent = buffer.str();
    return true;
}

void FileExplorer::updateFileColorBuffer() {
    fileColors.clear();
    fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    if (fileContent.size() != fileColors.size()) {
        std::cerr << "\033[35mFiles:\033[0m Error: Color buffer size mismatch" << std::endl;
        throw std::runtime_error("Color buffer size mismatch");
    }
}

void FileExplorer::setupUndoManager(const std::string& path) {
    auto it = fileUndoManagers.find(path);
    if (it == fileUndoManagers.end()) {
        it = fileUndoManagers.emplace(path, UndoRedoManager()).first;
        std::cout << "\033[35mFiles:\033[0m Created new UndoRedoManager for " << path << std::endl;
    }
    currentUndoManager = &(it->second);
    currentUndoManager->addState(fileContent, 0, fileContent.size());
}

void FileExplorer::initializeSyntaxHighlighting(const std::string& path) {
    std::string extension = fs::path(path).extension().string();
    gEditor.setLanguage(extension);
    gEditor.highlightContent(fileContent, fileColors, 0, fileContent.size());
}

void FileExplorer::handleLoadError() {
    fileContent = "Error: Unable to open file.";
    currentFile = "";
    fileColors.clear();
    currentUndoManager = nullptr;
}

void FileExplorer::loadFileContent(const std::string& path, std::function<void()> afterLoadCallback) {
    saveCurrentFile(); // Save current before loading new
    
    try {
        resetEditorState();
        
        if (!readFileContent(path)) {
            handleLoadError();
            return;
        }

        updateFilePathStates(path);
        updateFileColorBuffer();
        setupUndoManager(path);
        initializeSyntaxHighlighting(path);

        if (afterLoadCallback) {
            afterLoadCallback();
        }
        
        std::cout << "\033[35mFiles:\033[0m Loaded file: " << path << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "\033[35mFiles:\033[0m Error loading file: " << e.what() << std::endl;
        handleLoadError();
    }
}

void FileExplorer::findNext() {
  if (findText.empty())
    return;

  size_t startPos;
  if (lastFoundPos == std::string::npos) {
    startPos = editor_state.cursor_pos;
  } else {
    startPos = lastFoundPos + 1;
  }

  if (startPos >= fileContent.length())
    startPos = 0; // Wrap around if at end

  size_t foundPos = fileContent.find(findText, startPos);

  std::cout << "\033[35mFiles:\033[0m  Searching for '" << findText << "' starting from position "
            << startPos << std::endl;

  if (foundPos == std::string::npos) {
    // Wrap around to the beginning
    foundPos = fileContent.find(findText);
    std::cout << "\033[35mFiles:\033[0m  Wrapped search to beginning" << std::endl;
  }

  if (foundPos != std::string::npos) {
    lastFoundPos = foundPos;
    editor_state.cursor_pos = foundPos;
    editor_state.selection_start = foundPos;
    editor_state.selection_end = foundPos + findText.length();
    std::cout << "\033[35mFiles:\033[0m Found at position: " << foundPos
              << ", cursor now at: " << editor_state.cursor_pos << std::endl;
  } else {
    std::cout << "\033[35mFiles:\033[0m  Not found" << std::endl;
  }
}

void FileExplorer::findPrevious() {
  if (findText.empty())
    return;

  size_t startPos;
  if (lastFoundPos == std::string::npos) {
    startPos = editor_state.cursor_pos;
  } else {
    startPos =
        (lastFoundPos == 0) ? fileContent.length() - 1 : lastFoundPos - 1;
  }

  size_t foundPos = fileContent.rfind(findText, startPos);

  std::cout << "\033[35mFiles:\033[0m  Searching backwards for '" << findText
            << "' starting from position " << startPos << std::endl;

  if (foundPos == std::string::npos) {
    // Wrap around to the end
    foundPos = fileContent.rfind(findText);
    std::cout << "\033[35mFiles:\033[0m  Wrapped search to end" << std::endl;
  }

  if (foundPos != std::string::npos) {
    lastFoundPos = foundPos;
    editor_state.cursor_pos = foundPos;
    editor_state.selection_start = foundPos;
    editor_state.selection_end = foundPos + findText.length();
    std::cout << "\033[35mFiles:\033[0m  Found at position: " << foundPos
              << ", cursor now at: " << editor_state.cursor_pos << std::endl;
  } else {
    std::cout << "\033[35mFiles:\033[0m Not found" << std::endl;
  }
}

void FileExplorer::addUndoState(int changeStart, int changeEnd) {
  if (currentUndoManager) {
    currentUndoManager->addState(fileContent, changeStart, changeEnd);
  }
}

void FileExplorer::renderFileContent() {
  gLineJump.handleLineJumpInput(editor_state);
  gLineJump.renderLineJumpWindow(editor_state);

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

  bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
  bool cmd_pressed = ImGui::GetIO().KeySuper; // For macOS Command key
  if ((ctrl_pressed || cmd_pressed) && ImGui::IsKeyPressed(ImGuiKey_F)) {

    ClosePopper::closeAllExcept(ClosePopper::Type::LineJump);
    editor_state.activateFindBox = !editor_state.activateFindBox;
    editor_state.blockInput = editor_state.activateFindBox;
    if (editor_state.activateFindBox) {
      findText = "";
    }
  }

  if (editor_state.activateFindBox) {
    ImGui::SetNextItemWidth(-1);

    // Push styles for rounded borders and custom background
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);  // Rounded corners
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); // Border
    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Dark gray background
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Subtle border color

    static char inputBuffer[256] = "";
    ImGui::SetKeyboardFocusHere();
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                ImGuiInputTextFlags_AutoSelectAll;

    if (ImGui::InputText("##findbox", inputBuffer, sizeof(inputBuffer),
                         flags)) {
      findText = inputBuffer;
      lastFoundPos = std::string::npos; // Reset lastFoundPos for new search
      findNext();
    }

    // Pop the style variables and colors
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    bool shift_pressed = ImGui::GetIO().KeyShift;
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
      if (shift_pressed || cmd_pressed) {
        std::cout << "\033[35mFiles:\033[0m Searching previous" << std::endl;
        findPrevious();
      } else {
        std::cout << "\033[35mFiles:\033[0m  Searching next" << std::endl;
        findNext();
      }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      editor_state.activateFindBox = false;
      editor_state.blockInput = false;
    }
  }

  // Always render the editor
  bool text_changed =
      CustomTextEditor("##editor", fileContent, fileColors, editor_state);
  if (text_changed && !editor_state.activateFindBox) {
    setUnsavedChanges(true);
    std::cout << "\033[35mFiles:\033[0m  Text changed, added undo/redo state" << std::endl;
  }

  ImGui::PopStyleVar();
}

void FileExplorer::adjustColorBuffer(int changeStart, int lengthDiff) {
    if (lengthDiff > 0) {
        fileColors.insert(fileColors.begin() + changeStart, lengthDiff,
                       ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else if (lengthDiff < 0) {
        fileColors.erase(fileColors.begin() + changeStart,
                      fileColors.begin() + changeStart - lengthDiff);
    }
    fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void FileExplorer::rehighlightChangedRegion(int changeStart, int changeEnd) {
    int highlightStart = std::max(0, changeStart - 100);
    int highlightEnd = std::min(static_cast<int>(fileContent.size()), changeEnd + 100);
    
    std::string extension = fs::path(currentFile).extension().string();
    gEditor.setLanguage(extension);
    gEditor.highlightContent(fileContent, fileColors, highlightStart, highlightEnd);
}

void FileExplorer::applyContentChange(const UndoRedoManager::State& state, bool preAllocate) {
    if (preAllocate) {
        fileContent.reserve(std::max(fileContent.capacity(), 
                          state.content.length() + 1024 * 1024));
        fileColors.reserve(std::max(fileColors.capacity(), 
                         state.content.length() + 1024 * 1024));
    }

    int changeStart = std::min(state.changeStart, static_cast<int>(fileContent.length()));
    int changeEnd = std::min(state.changeEnd, static_cast<int>(fileContent.length()));
    
    int lengthDiff = state.content.length() - fileContent.length();
    fileContent = state.content;
    
    adjustColorBuffer(changeStart, lengthDiff);
    rehighlightChangedRegion(changeStart, changeEnd);
    _unsavedChanges = true;
}

void FileExplorer::handleUndo() {
    if (currentUndoManager) {
        auto state = currentUndoManager->undo(fileContent);
        applyContentChange(state);
    }
}

void FileExplorer::handleRedo() {
    if (currentUndoManager) {
        auto state = currentUndoManager->redo(fileContent);
        applyContentChange(state, true);  // Pre-allocate memory for redo
    }
}

void FileExplorer::saveCurrentFile() {
  if (!currentFile.empty() && _unsavedChanges) {
    std::ofstream file(currentFile);
		if (file.is_open()) {
		  file << fileContent;
		  file.close();
		  _unsavedChanges = false;
		  std::cout << "\033[35mFiles:\033[0m  File saved: " << currentFile << std::endl;
		} else {
      std::cerr << "\033[35mFiles:\033[0m  Unable to save file: " << currentFile << std::endl;
    }
  }
}

