#define GL_SILENCE_DEPRECATION
#include "files.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "util/settings.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nfd.h>
#include <sstream>
#define NANOSVG_IMPLEMENTATION
#include "lib/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "lib/nanosvgrast.h"
#include "util/line_jump.h"


FileExplorer gFileExplorer;
void FileExplorer::loadIcons() {
  std::vector<std::string> iconFiles = {
                                        "close.svg",
                                        "gear.svg",
                                        "gear-hover.svg",
                                        "py.svg",
                                        "h.svg",
                                        "hpp.svg",
                                        "gitignore.svg",
                                        "js.svg",
                                        "R.svg",
                                        "apple.svg",
                                        "argdown.svg",
                                        "asm.svg",
                                        "audio.svg",
                                        "babel.svg",
                                        "bazel.svg",
                                        "bicep.svg",
                                        "bower.svg",
                                        "bsl.svg",
                                        "c-sharp.svg",
                                        "c.svg",
                                        "cake.svg",
                                        "cake_php.svg",
                                        "checkbox-unchecked.svg",
                                        "checkbox.svg",
                                        "cjsx.svg",
                                        "clock.svg",
                                        "clojure.svg",
                                        "code-climate.svg",
                                        "code-search.svg",
                                        "coffee.svg",
                                        "coffee_erb.svg",
                                        "coldfusion.svg",
                                        "config.svg",
                                        "cpp.svg",
                                        "crystal.svg",
                                        "crystal_embedded.svg",
                                        "css.svg",
                                        "csv.svg",
                                        "cu.svg",
                                        "d.svg",
                                        "dart.svg",
                                        "db.svg",
                                        "default.svg",
                                        "deprecation-cop.svg",
                                        "docker.svg",
                                        "editorconfig.svg",
                                        "ejs.svg",
                                        "elixir.svg",
                                        "elixir_script.svg",
                                        "elm.svg",
                                        "error.svg",
                                        "eslint.svg",
                                        "ethereum.svg",
                                        "f-sharp.svg",
                                        "favicon.svg",
                                        "firebase.svg",
                                        "firefox.svg",
                                        "font.svg",
                                        "git.svg",
                                        "git_folder.svg",
                                        "folder.svg",
                                        "folder-open.svg",
                                        "git_ignore.svg",
                                        "github.svg",
                                        "gitlab.svg",
                                        "go.svg",
                                        "go2.svg",
                                        "godot.svg",
                                        "gradle.svg",
                                        "grails.svg",
                                        "graphql.svg",
                                        "grunt.svg",
                                        "gulp.svg",
                                        "hacklang.svg",
                                        "haml.svg",
                                        "happenings.svg",
                                        "haskell.svg",
                                        "haxe.svg",
                                        "heroku.svg",
                                        "hex.svg",
                                        "html.svg",
                                        "html_erb.svg",
                                        "ignored.svg",
                                        "illustrator.svg",
                                        "image.svg",
                                        "info.svg",
                                        "ionic.svg",
                                        "jade.svg",
                                        "java.svg",
                                        "javascript.svg",
                                        "jenkins.svg",
                                        "jinja.svg",
                                        "js_erb.svg",
                                        "json.svg",
                                        "julia.svg",
                                        "karma.svg",
                                        "kotlin.svg",
                                        "less.svg",
                                        "license.svg",
                                        "liquid.svg",
                                        "livescript.svg",
                                        "lock.svg",
                                        "lua.svg",
                                        "makefile.svg",
                                        "markdown.svg",
                                        "maven.svg",
                                        "mdo.svg",
                                        "mustache.svg",
                                        "new-file.svg",
                                        "nim.svg",
                                        "notebook.svg",
                                        "npm.svg",
                                        "npm_ignored.svg",
                                        "nunjucks.svg",
                                        "ocaml.svg",
                                        "odata.svg",
                                        "pddl.svg",
                                        "pdf.svg",
                                        "perl.svg",
                                        "photoshop.svg",
                                        "php.svg",
                                        "pipeline.svg",
                                        "plan.svg",
                                        "platformio.svg",
                                        "powershell.svg",
                                        "prisma.svg",
                                        "project.svg",
                                        "prolog.svg",
                                        "pug.svg",
                                        "puppet.svg",
                                        "purescript.svg",
                                        "python.svg",
                                        "rails.svg",
                                        "react.svg",
                                        "reasonml.svg",
                                        "rescript.svg",
                                        "rollup.svg",
                                        "ruby.svg",
                                        "rust.svg",
                                        "salesforce.svg",
                                        "sass.svg",
                                        "sbt.svg",
                                        "scala.svg",
                                        "search.svg",
                                        "settings.svg",
                                        "shell.svg",
                                        "slim.svg",
                                        "smarty.svg",
                                        "spring.svg",
                                        "stylelint.svg",
                                        "stylus.svg",
                                        "sublime.svg",
                                        "svelte.svg",
                                        "svg.svg",
                                        "swift.svg",
                                        "terraform.svg",
                                        "tex.svg",
                                        "time-cop.svg",
                                        "todo.svg",
                                        "tsconfig.svg",
                                        "twig.svg",
                                        "typescript.svg",
                                        "vala.svg",
                                        "video.svg",
                                        "vue.svg",
                                        "wasm.svg",
                                        "wat.svg",
                                        "webpack.svg",
                                        "wgt.svg",
                                        "windows.svg",
                                        "word.svg",
                                        "xls.svg",
                                        "xml.svg",
                                        "yarn.svg",
                                        "yml.svg",
                                        "zig.svg",
                                        "zip.svg"};

  for (const auto &iconFile : iconFiles) {
    std::string fullPath = "icons/" + iconFile;
    if (!std::filesystem::exists(fullPath)) {
      //std::cout << "Files: Icon file does not exist: " << fullPath << std::endl;
      continue;
    }

    NSVGimage *image = nsvgParseFromFile(fullPath.c_str(), "px", 96.0f);
    if (image == nullptr) {
      std::cerr << "Error loading SVG file: " << fullPath << std::endl;
      continue;
    }

    int width = 32;  // Increased size for better quality
    int height = 32; // Increased size for better quality

    NSVGrasterizer *rast = nsvgCreateRasterizer();
    if (rast == nullptr) {
      std::cerr << "Error creating SVG rasterizer" << std::endl;
      nsvgDelete(image);
      continue;
    }

    unsigned char *pixels = new unsigned char[width * height * 4];
    nsvgRasterize(rast, image, 0, 0, width / image->width, pixels, width,
                  height, width * 4);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);

    std::string iconName = iconFile.substr(0, iconFile.find('.'));
    fileTypeIcons[iconName] =
        reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));

    delete[] pixels;
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
  }
  // Create a default icon if no icons were loaded
  if (fileTypeIcons.empty()) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    unsigned char defaultIcon[] = {
        255, 255, 255, 255, // White pixel
        0,   0,   0,   255  // Black pixel
    };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 defaultIcon);
    fileTypeIcons["default"] =
        reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
  }
}

ImTextureID FileExplorer::getIconForFile(const std::string &filename) {
  std::string extension = fs::path(filename).extension().string();
  if (!extension.empty() && extension[0] == '.') {
    extension = extension.substr(1);
  }
  if (fileTypeIcons.find(extension) != fileTypeIcons.end()) {
    return fileTypeIcons[extension];
  }
  return fileTypeIcons["default"]; // Use default icon if specific icon not
                                   // found
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
void FileExplorer::displayFileTree(FileNode &node) {
    float currentFontSize = gSettings.getFontSize();
    
    // Set icon sizes based on font size
    float folderIconSize = currentFontSize * .8f;  // 20% larger than font
    float fileIconSize = currentFontSize * 1.2f;    // Same as folder for consistency
    
    static bool printedSizes = false;  // To prevent spamming console
    if (printedSizes) {
        std::cout << "Files: Font Size: " << currentFontSize << std::endl;
        std::cout << "Files: Folder Icon Size: " << folderIconSize << std::endl;
        std::cout << "FIles: File Icon Size: " << fileIconSize << std::endl;
        printedSizes = true;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 3));
    ImGui::PushID(node.fullPath.c_str());
    float item_height = ImGui::GetFrameHeight();
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    
    static int current_depth = 0;
    float indent_width = 28.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + current_depth * indent_width);
    
    if (node.isDirectory) {
      ImVec2 iconSize(folderIconSize, folderIconSize);
      // Calculate vertical centering
      float verticalPadding = (item_height - iconSize.y) * 0.5f;  // Center vertically
      float horizontalPadding = 8.0f;  // Add some left/right padding

      // Select folder icon based on open state
      ImTextureID folderIcon;
      if (node.isOpen) {
          folderIcon = fileTypeIcons["folder-open"];
          if (!folderIcon) {
              folderIcon = fileTypeIcons["folder"];
          }
      } else {
          folderIcon = fileTypeIcons["folder"];
      }
      
      if (!folderIcon) {
          folderIcon = fileTypeIcons["default"];
      }

      // Folder button with custom styling - add padding to the button
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.31f));
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(horizontalPadding, verticalPadding));
      bool isOpen = ImGui::Button(("##" + node.fullPath).c_str(), 
          ImVec2(ImGui::GetContentRegionAvail().x, item_height));
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
      
      // Draw icon with adjusted position
      ImGui::SetCursorPos(ImVec2(
          cursor_pos.x + current_depth * indent_width + horizontalPadding,  // Add horizontal padding
          cursor_pos.y + verticalPadding  // Add vertical centering
      ));
      ImGui::Image(folderIcon, iconSize);
      
      // Draw folder name with more spacing after icon
      float textPadding = 14.0f;  // Space between icon and text
      ImGui::SameLine(current_depth * indent_width + iconSize.x + horizontalPadding + textPadding);
      ImGui::SetCursorPosY(cursor_pos.y + (item_height - ImGui::GetTextLineHeight()) * 0.5f);  // Center text vertically
      ImGui::Text("%s", node.name.c_str());
        
        if (isOpen) {
            node.isOpen = !node.isOpen;
            if (node.isOpen) {
                buildFileTree(node.fullPath, node);
            }
        }
        
        // Recursively display children if open
        if (node.isOpen) {
            current_depth++;
            for (auto &child : node.children) {
                displayFileTree(child);
            }
            current_depth--;
        }
    } else {
        ImVec2 iconSize(fileIconSize, fileIconSize);
        ImTextureID fileIcon = getIconForFile(node.name);
        float leftMargin = 3.0f;  // Add margin before the icon
        
        // File button with custom styling
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.31f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        bool clicked = ImGui::Button(("##" + node.fullPath).c_str(), 
            ImVec2(ImGui::GetContentRegionAvail().x, item_height));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        
        // Draw icon with left margin
        ImGui::SetCursorPos(cursor_pos);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + current_depth * indent_width + leftMargin);
        ImGui::Image(fileIcon, iconSize);
        
        // Draw file name (adjust the padding to account for the new margin)
        ImGui::SameLine(current_depth * indent_width + iconSize.x + leftMargin + 10);
        
        if (node.fullPath == currentOpenFile) {
            if (gSettings.getRainbowMode()) {
                // Rainbow mode - use animated rainbow color
                ImVec4 fileColor = GetRainbowColor(ImGui::GetTime() * 2.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, fileColor);
                ImGui::Text("%s", node.name.c_str());
                ImGui::PopStyleColor();
            } else {
                // Non-rainbow mode - use a static highlight color
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f)); // Light gray
                ImGui::Text("%s", node.name.c_str());
                ImGui::PopStyleColor();
            }
        } else {
            ImGui::Text("%s", node.name.c_str());
        }
        
        if (clicked) {
            loadFileContent(node.fullPath);
        }
    }
    ImGui::PopID();
    ImGui::PopStyleVar(3);
}

void FileExplorer::openFolderDialog() {
  nfdchar_t *outPath = NULL;
  nfdresult_t result = NFD_PickFolder(NULL, &outPath);
  if (result == NFD_OKAY) {
    selectedFolder = outPath;
    std::cout << "\033[35mFiles:\033[0m Selected folder: " << outPath << std::endl;
    free(outPath);
    _showFileDialog = false;
  } else if (result == NFD_CANCEL) {
    std::cout << "\033[35mFiles:\033[0m User canceled folder selection." << std::endl;
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

void FileExplorer::loadFileContent(const std::string& path, std::function<void()> afterLoadCallback) {
  saveCurrentFile(); // Save the current file before loading a new one
  gEditor.cancelHighlighting(); // Cancel any ongoing highlighting
  editor_state.cursor_pos = 0;
  editor_state.current_line = 0;

  std::ifstream file(path);
  if (file) {
    std::stringstream buffer;
    buffer << file.rdbuf();
    fileContent = buffer.str();
    currentFile = path;
    if (currentOpenFile != path) {
      previousOpenFile = currentOpenFile;
      currentOpenFile = path;
    }
    _unsavedChanges = false;

    // Resize fileColors to match fileContent
    fileColors.clear();
    fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    if (fileContent.size() != fileColors.size()) {
      std::cerr << "\033[35mFiles:\033[0m  Error: Mismatch between fileContent and fileColors size "
                   "after loading"
                << std::endl;
      return;
    }
    auto it = fileUndoManagers.find(path);
    if (it == fileUndoManagers.end()) {
      it = fileUndoManagers.emplace(path, UndoRedoManager()).first;
      std::cout << "\033[35mFiles:\033[0m  Created new UndoRedoManager for " << path << std::endl;
    } else {
      std::cout << "\033[35mFiles:\033[0m  Using existing UndoRedoManager for " << path << std::endl;
    }
    currentUndoManager = &(it->second);
    currentUndoManager->addState(fileContent, 0, fileContent.size());

    std::string extension = fs::path(path).extension().string();
    gEditor.setLanguage(extension);

    // Check if fileContent and fileColors have the same size before
    // highlighting
    if (fileContent.size() == fileColors.size()) {
      gEditor.highlightContent(fileContent, fileColors, 0, fileContent.size());
    } else {
      std::cerr << "Error: fileContent and fileColors size mismatch"
                << std::endl;
    }
    if (afterLoadCallback) {
        afterLoadCallback();
    }
    std::cout << "\033[35mFiles:\033[0m  Loaded file: " << path << std::endl;
  } else {
    fileContent = "Error: Unable to open file.";
    currentFile = "";
    fileColors.clear();
    currentUndoManager = nullptr;
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
void FileExplorer::handleUndo() {
  if (currentUndoManager) {
    auto state = currentUndoManager->undo(fileContent);
    int changeStart = state.changeStart;
    int changeEnd = state.changeEnd;

    // Calculate the difference in length
    int lengthDiff = state.content.length() - fileContent.length();

    fileContent = state.content;

    // Adjust the colors vector
    if (lengthDiff > 0) {
      // Text was added, insert default colors
      fileColors.insert(fileColors.begin() + changeStart, lengthDiff,
                        ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else if (lengthDiff < 0) {
      // Text was removed, erase colors
      fileColors.erase(fileColors.begin() + changeStart,
                       fileColors.begin() + changeStart - lengthDiff);
    }

    // Ensure colors vector matches content size
    fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Rehighlight the changed portion and a bit more for context
    int highlightStart = std::max(0, changeStart - 100);
    int highlightEnd =
        std::min(static_cast<int>(fileContent.size()), changeEnd + 100);

    std::string extension = fs::path(currentFile).extension().string();
    gEditor.setLanguage(extension);
    gEditor.highlightContent(fileContent, fileColors, highlightStart,
                             highlightEnd);

    _unsavedChanges = true;
  }
}
void FileExplorer::handleRedo() {
  if (currentUndoManager) {
    auto state = currentUndoManager->redo(fileContent);

    // Pre-allocate more memory
    fileContent.reserve(
        std::max(fileContent.capacity(),
                 state.content.length() + 1024 * 1024)); // Extra 1MB
    fileColors.reserve(
        std::max(fileColors.capacity(), state.content.length() + 1024 * 1024));

    int changeStart =
        std::min(state.changeStart, static_cast<int>(fileContent.length()));
    int changeEnd =
        std::min(state.changeEnd, static_cast<int>(fileContent.length()));

    // Calculate the difference in length
    int lengthDiff = static_cast<int>(state.content.length()) -
                     static_cast<int>(fileContent.length());

    fileContent = state.content;

    // Adjust the colors vector
    if (lengthDiff > 0) {
      // Text was added, insert default colors
      fileColors.insert(fileColors.begin() + changeStart, lengthDiff,
                        ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else if (lengthDiff < 0) {
      // Text was removed, erase colors
      fileColors.erase(fileColors.begin() + changeStart,
                       fileColors.begin() +
                           std::min(changeStart - lengthDiff,
                                    static_cast<int>(fileColors.size())));
    }

    // Ensure colors vector matches content size
    fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Rehighlight the changed portion and a bit more for context
    int highlightStart = std::max(0, changeStart - 100);
    int highlightEnd =
        std::min(static_cast<int>(fileContent.size()), changeEnd + 100);

    std::string extension = fs::path(currentFile).extension().string();
    gEditor.setLanguage(extension);
    gEditor.highlightContent(fileContent, fileColors, highlightStart,
                             highlightEnd);

    _unsavedChanges = true;
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