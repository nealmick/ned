#define GL_SILENCE_DEPRECATION

#include "files.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <nfd.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "settings.h"
#define NANOSVG_IMPLEMENTATION
#include "lib/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "lib/nanosvgrast.h"


FileExplorer gFileExplorer;
void FileExplorer::loadIcons() {
    std::vector<std::string> iconFiles = {
        "py.svg","h.svg","hpp.svg","gitignore.svg","js.svg","R.svg", "apple.svg", "argdown.svg", "asm.svg", "audio.svg",
        "babel.svg", "bazel.svg", "bicep.svg", "bower.svg", "bsl.svg",
        "c-sharp.svg", "c.svg", "cake.svg", "cake_php.svg", "checkbox-unchecked.svg",
        "checkbox.svg", "cjsx.svg", "clock.svg", "clojure.svg", "code-climate.svg",
        "code-search.svg", "coffee.svg", "coffee_erb.svg", "coldfusion.svg", "config.svg",
        "cpp.svg", "crystal.svg", "crystal_embedded.svg", "css.svg", "csv.svg",
        "cu.svg", "d.svg", "dart.svg", "db.svg", "default.svg",
        "deprecation-cop.svg", "docker.svg", "editorconfig.svg", "ejs.svg", "elixir.svg",
        "elixir_script.svg", "elm.svg", "error.svg", "eslint.svg", "ethereum.svg",
        "f-sharp.svg", "favicon.svg", "firebase.svg", "firefox.svg", "font.svg",
        "git.svg", "git_folder.svg", "git_ignore.svg", "github.svg", "gitlab.svg",
        "go.svg", "go2.svg", "godot.svg", "gradle.svg", "grails.svg",
        "graphql.svg", "grunt.svg", "gulp.svg", "hacklang.svg", "haml.svg",
        "happenings.svg", "haskell.svg", "haxe.svg", "heroku.svg", "hex.svg",
        "html.svg", "html_erb.svg", "ignored.svg", "illustrator.svg", "image.svg",
        "info.svg", "ionic.svg", "jade.svg", "java.svg", "javascript.svg",
        "jenkins.svg", "jinja.svg", "js_erb.svg", "json.svg", "julia.svg",
        "karma.svg", "kotlin.svg", "less.svg", "license.svg", "liquid.svg",
        "livescript.svg", "lock.svg", "lua.svg", "makefile.svg", "markdown.svg",
        "maven.svg", "mdo.svg", "mustache.svg", "new-file.svg", "nim.svg",
        "notebook.svg", "npm.svg", "npm_ignored.svg", "nunjucks.svg", "ocaml.svg",
        "odata.svg", "pddl.svg", "pdf.svg", "perl.svg", "photoshop.svg",
        "php.svg", "pipeline.svg", "plan.svg", "platformio.svg", "powershell.svg",
        "prisma.svg", "project.svg", "prolog.svg", "pug.svg", "puppet.svg",
        "purescript.svg", "python.svg", "rails.svg", "react.svg", "reasonml.svg",
        "rescript.svg", "rollup.svg", "ruby.svg", "rust.svg", "salesforce.svg",
        "sass.svg", "sbt.svg", "scala.svg", "search.svg", "settings.svg",
        "shell.svg", "slim.svg", "smarty.svg", "spring.svg", "stylelint.svg",
        "stylus.svg", "sublime.svg", "svelte.svg", "svg.svg", "swift.svg",
        "terraform.svg", "tex.svg", "time-cop.svg", "todo.svg", "tsconfig.svg",
        "twig.svg", "typescript.svg", "vala.svg", "video.svg", "vue.svg",
        "wasm.svg", "wat.svg", "webpack.svg", "wgt.svg", "windows.svg",
        "word.svg", "xls.svg", "xml.svg", "yarn.svg", "yml.svg",
        "zig.svg", "zip.svg"
    };

    for (const auto& iconFile : iconFiles) {
        std::string fullPath = "icons/" + iconFile;
        if (!std::filesystem::exists(fullPath)) {
            std::cout << "Icon file does not exist: " << fullPath << std::endl;
            continue;
        }
        
        NSVGimage* image = nsvgParseFromFile(fullPath.c_str(), "px", 96.0f);
        if (image == nullptr) {
            std::cerr << "Error loading SVG file: " << fullPath << std::endl;
            continue;
        }

        int width = 32;  // Increased size for better quality
        int height = 32; // Increased size for better quality

        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if (rast == nullptr) {
            std::cerr << "Error creating SVG rasterizer" << std::endl;
            nsvgDelete(image);
            continue;
        }

        unsigned char* pixels = new unsigned char[width * height * 4];
        nsvgRasterize(rast, image, 0, 0, width / image->width, pixels, width, height, width * 4);

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        std::string iconName = iconFile.substr(0, iconFile.find('.'));
        fileTypeIcons[iconName] = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));

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
            255, 255, 255, 255,  // White pixel
            0, 0, 0, 255         // Black pixel
        };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultIcon);
        fileTypeIcons["default"] = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
    }
}




ImTextureID FileExplorer::getIconForFile(const std::string& filename) {
    std::string extension = fs::path(filename).extension().string();
    if (!extension.empty() && extension[0] == '.') {
        extension = extension.substr(1);
    }
    if (fileTypeIcons.find(extension) != fileTypeIcons.end()) {
        return fileTypeIcons[extension];
    }
    return fileTypeIcons["default"]; // Use default icon if specific icon not found
}
void FileExplorer::buildFileTree(const fs::path& path, FileNode& node) {
    std::vector<FileNode> newChildren;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            FileNode child;
            child.name = entry.path().filename().string();
            child.fullPath = entry.path().string();
            child.isDirectory = entry.is_directory();
            
            // Check if this child already exists and preserve its open state
            auto it = std::find_if(node.children.begin(), node.children.end(),
                [&child](const FileNode& existingChild) {
                    return existingChild.fullPath == child.fullPath;
                });
            if (it != node.children.end()) {
                child.isOpen = it->isOpen;
                if (child.isDirectory && child.isOpen) {
                    buildFileTree(child.fullPath, child);
                }
            }
            
            newChildren.push_back(std::move(child));
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory " << path << ": " << e.what() << std::endl;
    }
    
    node.children = std::move(newChildren);
    
    std::sort(node.children.begin(), node.children.end(),
              [](const FileNode& a, const FileNode& b) {
                  if (a.isDirectory != b.isDirectory) {
                      return a.isDirectory > b.isDirectory;
                  }
                  return a.name < b.name;
              });
}
void FileExplorer::preserveOpenStates(const FileNode& oldNode, FileNode& newNode) {
    for (auto& newChild : newNode.children) {
        auto it = std::find_if(oldNode.children.begin(), oldNode.children.end(),
            [&newChild](const FileNode& oldChild) {
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
    if (!selectedFolder.empty()) {
        rootNode.name = fs::path(selectedFolder).filename().string();
        rootNode.fullPath = selectedFolder;
        rootNode.isDirectory = true;
        rootNode.children.clear();
        buildFileTree(selectedFolder, rootNode);
    }
}


void FileExplorer::displayFileTree(FileNode& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (!node.isDirectory) flags |= ImGuiTreeNodeFlags_Leaf;

    ImGui::PushID(node.fullPath.c_str());

    if (node.isDirectory) {
        bool isOpened = ImGui::TreeNodeEx(node.name.c_str(), flags, "%s", node.name.c_str());

        if (ImGui::IsItemClicked() && node.children.empty()) {
            buildFileTree(node.fullPath, node);
        }

        if (isOpened != node.isOpen) {
            node.isOpen = isOpened;
            if (isOpened && node.children.empty()) {
                buildFileTree(node.fullPath, node);
            }
        }

        if (isOpened) {
            for (auto& child : node.children) {
                displayFileTree(child);
            }
            ImGui::TreePop();
        }
    } else {
        ImTextureID icon = getIconForFile(node.name);
        
        float icon_width = 20.0f;
        float icon_spacing = 6.0f;
        float tree_node_spacing = ImGui::GetTreeNodeToLabelSpacing();
        float adjusted_indent = tree_node_spacing - (icon_width + icon_spacing);

        ImGui::Indent(adjusted_indent);
        
        ImGui::BeginGroup();
        
        bool selected = false;
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0, 0.5f));
        ImGui::Selectable("##hidden", &selected, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight()));
        ImGui::PopStyleVar();

        bool clicked = ImGui::IsItemClicked();

        ImGui::SameLine(0, 0);
        ImGui::SetCursorPosX(ImGui::GetItemRectMin().x);
        ImGui::Image(icon, ImVec2(icon_width, icon_width));
        ImGui::SameLine(0, icon_spacing);
        bool useRainbowEffect = true;
        // Use rainbow color for the active file
        if (node.fullPath == currentOpenFile) {
        ImVec4 fileColor;
        if (useRainbowEffect) {
            static float last_update_time = 0.0f;
            static ImVec4 last_rainbow_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            float current_time = ImGui::GetTime();
            
            if (current_time - last_update_time > 0.05f) {
                last_rainbow_color = GetRainbowColor(current_time * 2.0f);
                last_update_time = current_time;
            }
            fileColor = last_rainbow_color;
        } else {
            fileColor = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);//non rainbow active file color
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, fileColor);
    }
        
        ImGui::TextUnformatted(node.name.c_str());
        
        if (node.fullPath == currentOpenFile) {
            ImGui::PopStyleColor();
        }
        
        ImGui::EndGroup();

        if (clicked) {
            loadFileContent(node.fullPath);
        }
        
        ImGui::Unindent(adjusted_indent);
    }

    ImGui::PopID();
}


void FileExplorer::openFolderDialog() {
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_PickFolder(NULL, &outPath);

    if (result == NFD_OKAY) {
        selectedFolder = outPath;
        std::cout << "Selected folder: " << outPath << std::endl;
        free(outPath);
        _showFileDialog = false;
    } else if (result == NFD_CANCEL) {
        std::cout << "User canceled folder selection." << std::endl;
    } else {
        std::cout << "Error: " << NFD_GetError() << std::endl;
    }
}


void FileExplorer::refreshSyntaxHighlighting() {
    if (!currentFile.empty()) {
        std::string extension = fs::path(currentFile).extension().string();
        gEditor.setLanguage(extension);
        gEditor.highlightContent(fileContent, fileColors, 0, fileContent.size());
    }
}

void FileExplorer::loadFileContent(const std::string& path) {
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
            std::cerr << "Error: Mismatch between fileContent and fileColors size after loading" << std::endl;
            return;
        }
        auto it = fileUndoManagers.find(path);
        if (it == fileUndoManagers.end()) {
            it = fileUndoManagers.emplace(path, UndoRedoManager()).first;
            std::cout << "Created new UndoRedoManager for " << path << std::endl;
        } else {
            std::cout << "Using existing UndoRedoManager for " << path << std::endl;
        }
        currentUndoManager = &(it->second);
        currentUndoManager->addState(fileContent, 0, fileContent.size());
        
        std::string extension = fs::path(path).extension().string();
        gEditor.setLanguage(extension);
        
        // Check if fileContent and fileColors have the same size before highlighting
        if (fileContent.size() == fileColors.size()) {
            gEditor.highlightContent(fileContent, fileColors, 0, fileContent.size());
        } else {
            std::cerr << "Error: fileContent and fileColors size mismatch" << std::endl;
        }

        std::cout << "Loaded file: " << path << std::endl;
        std::cout << "File size: " << fileContent.size() << std::endl;
        std::cout << "Colors size: " << fileColors.size() << std::endl;
    } else {
        fileContent = "Error: Unable to open file.";
        currentFile = "";
        fileColors.clear();
        currentUndoManager = nullptr;
    }
}
void FileExplorer::findNext() {
    if (findText.empty()) return;
    
    size_t startPos;
    if (lastFoundPos == std::string::npos) {
        startPos = editor_state.cursor_pos;
    } else {
        startPos = lastFoundPos + 1;
    }
    
    if (startPos >= fileContent.length()) startPos = 0;  // Wrap around if at end
    
    size_t foundPos = fileContent.find(findText, startPos);
    
    std::cout << "Searching for '" << findText << "' starting from position " << startPos << std::endl;
    
    if (foundPos == std::string::npos) {
        // Wrap around to the beginning
        foundPos = fileContent.find(findText);
        std::cout << "Wrapped search to beginning" << std::endl;
    }
    
    if (foundPos != std::string::npos) {
        lastFoundPos = foundPos;
        editor_state.cursor_pos = foundPos;
        editor_state.selection_start = foundPos;
        editor_state.selection_end = foundPos + findText.length();
        std::cout << "Found at position: " << foundPos << ", cursor now at: " << editor_state.cursor_pos << std::endl;
    } else {
        std::cout << "Not found" << std::endl;
    }
}

void FileExplorer::findPrevious() {
    if (findText.empty()) return;
    
    size_t startPos;
    if (lastFoundPos == std::string::npos) {
        startPos = editor_state.cursor_pos;
    } else {
        startPos = (lastFoundPos == 0) ? fileContent.length() - 1 : lastFoundPos - 1;
    }
    
    size_t foundPos = fileContent.rfind(findText, startPos);
    
    std::cout << "Searching backwards for '" << findText << "' starting from position " << startPos << std::endl;
    
    if (foundPos == std::string::npos) {
        // Wrap around to the end
        foundPos = fileContent.rfind(findText);
        std::cout << "Wrapped search to end" << std::endl;
    }
    
    if (foundPos != std::string::npos) {
        lastFoundPos = foundPos;
        editor_state.cursor_pos = foundPos;
        editor_state.selection_start = foundPos;
        editor_state.selection_end = foundPos + findText.length();
        std::cout << "Found at position: " << foundPos << ", cursor now at: " << editor_state.cursor_pos << std::endl;
    } else {
        std::cout << "Not found" << std::endl;
    }
}


void FileExplorer::addUndoState(int changeStart, int changeEnd) {
    if (currentUndoManager) {
        currentUndoManager->addState(fileContent, changeStart, changeEnd);
    }
}
void FileExplorer::renderFileContent() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool cmd_pressed = ImGui::GetIO().KeySuper;  // For macOS Command key
    if ((ctrl_pressed || cmd_pressed) && ImGui::IsKeyPressed(ImGuiKey_F)) {
        editor_state.activateFindBox = !editor_state.activateFindBox;
        editor_state.blockInput = editor_state.activateFindBox;
        if (editor_state.activateFindBox) {
            findText = "";
        }
    }
    
    if (editor_state.activateFindBox) {
        ImGui::SetNextItemWidth(-1);
        
        // Push the new style color for the input text background
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Dark gray background
        
        static char inputBuffer[256] = "";
        ImGui::SetKeyboardFocusHere();
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
        
        if (ImGui::InputText("##findbox", inputBuffer, sizeof(inputBuffer), flags)) {
            findText = inputBuffer;
            lastFoundPos = std::string::npos;  // Reset lastFoundPos for new search
            findNext();
        }
        
        // Pop the style color to restore the original color
        ImGui::PopStyleColor();
        
        bool shift_pressed = ImGui::GetIO().KeyShift;
        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
            if (shift_pressed || cmd_pressed) {
                std::cout << "Searching previous" << std::endl;
                findPrevious();
            } else {
                std::cout << "Searching next" << std::endl;
                findNext();
            }
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            editor_state.activateFindBox = false;
            editor_state.blockInput = false;
        }
    }

    // Always render the editor
    bool text_changed = CustomTextEditor("##editor", fileContent, fileColors, editor_state);
    if (text_changed && !editor_state.activateFindBox) {
        setUnsavedChanges(true);
        std::cout << "Text changed, added undo state" << std::endl;
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
            fileColors.insert(fileColors.begin() + changeStart, lengthDiff, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else if (lengthDiff < 0) {
            // Text was removed, erase colors
            fileColors.erase(fileColors.begin() + changeStart, fileColors.begin() + changeStart - lengthDiff);
        }
        
        // Ensure colors vector matches content size
        fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        // Rehighlight the changed portion and a bit more for context
        int highlightStart = std::max(0, changeStart - 100);
        int highlightEnd = std::min(static_cast<int>(fileContent.size()), changeEnd + 100);
        
        std::string extension = fs::path(currentFile).extension().string();
        gEditor.setLanguage(extension);
        gEditor.highlightContent(fileContent, fileColors, highlightStart, highlightEnd);
        
        _unsavedChanges = true;
    }
}
void FileExplorer::handleRedo() {
    if (currentUndoManager) {
        auto state = currentUndoManager->redo(fileContent);
        
        // Pre-allocate more memory
        fileContent.reserve(std::max(fileContent.capacity(), state.content.length() + 1024 * 1024)); // Extra 1MB
        fileColors.reserve(std::max(fileColors.capacity(), state.content.length() + 1024 * 1024));

        int changeStart = std::min(state.changeStart, static_cast<int>(fileContent.length()));
        int changeEnd = std::min(state.changeEnd, static_cast<int>(fileContent.length()));
        
        // Calculate the difference in length
        int lengthDiff = static_cast<int>(state.content.length()) - static_cast<int>(fileContent.length());
        
        fileContent = state.content;
        
        // Adjust the colors vector
        if (lengthDiff > 0) {
            // Text was added, insert default colors
            fileColors.insert(fileColors.begin() + changeStart, lengthDiff, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else if (lengthDiff < 0) {
            // Text was removed, erase colors
            fileColors.erase(fileColors.begin() + changeStart, fileColors.begin() + std::min(changeStart - lengthDiff, static_cast<int>(fileColors.size())));
        }
        
        // Ensure colors vector matches content size
        fileColors.resize(fileContent.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        // Rehighlight the changed portion and a bit more for context
        int highlightStart = std::max(0, changeStart - 100);
        int highlightEnd = std::min(static_cast<int>(fileContent.size()), changeEnd + 100);
        
        std::string extension = fs::path(currentFile).extension().string();
        gEditor.setLanguage(extension);
        gEditor.highlightContent(fileContent, fileColors, highlightStart, highlightEnd);
        
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
            std::cout << "File saved: " << currentFile << std::endl;
        } else {
            std::cerr << "Unable to save file: " << currentFile << std::endl;
        }
    }
}