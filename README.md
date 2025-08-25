<img width="250" height="150" alt="ned-3" src="https://github.com/user-attachments/assets/49cc36bf-1164-4bdc-bc22-7b89a0021c77" />

<img src="https://github.com/nealmick/ned/actions/workflows/pack-app.yml/badge.svg" alt="Build Status">  ![macOS](https://img.shields.io/badge/macOS-✓-success?logo=apple&logoColor=white)  ![Debian](https://img.shields.io/badge/Debian-✓-success?logo=debian&logoColor=white)  ![Windows](https://img.shields.io/badge/Windows-✓-success?logo=microsoft&logoColor=white)

A retro-style text editor with GL shader effects. NED offers Tree Sitter syntax highlighting, LSP integration, and a terminal emulator.


# [Download](https://github.com/nealmick/ned/releases)

https://github.com/user-attachments/assets/74af4120-7cf7-4e8c-9b60-7e2aa3228be0

## Shader Effects:  Static Noise, Burn In, Screen Curvature, Bloom, Vignetting. 
| Amber | Solarized |
|---------|---------|
| <video src="https://github.com/user-attachments/assets/1461119f-8eef-4fe0-8564-c49ab1d0b227" width="300"></video> | <video src="https://github.com/user-attachments/assets/465204eb-cd81-4621-8a03-e5319b8a9103" width="300"></video> |

| Ned | Custom |
|---------|---------|
| <video src="https://github.com/user-attachments/assets/9f352517-2c51-4fa2-a008-84c254175326" width="300"></video> | <video src="https://github.com/user-attachments/assets/86c6810e-2507-440c-80cd-467df04483ce" width="300"></video> |




#### Notable Features
- OpenGL Shaders with retro style for the best coding vibes
- Text Bookmarks make editing multiple files with saved cursors a breeze
- Rainbow mode cursor so you never lose your cursor and stand out
- LSP Adapters for easy navigation and advanced language support
- Terminal Emulator based on suckless st.c ported to C++ with multiplexer support
- Optional Custom lexers and tokenizers for custom languages and obscure syntax patterns
- Copilot-like auto complete using OpenRouter, choose the latest and best LLM models
- Multi-cursor support, easily find and replace strings with multi selection



## Build from source
#### Prerequisites
CMake (version 3.10 or higher)
C++20 compatible compiler
OpenGL
GLFW3
Glew
Curl

Clone the repository with its submodules:
```sh
#Make sure you clone with recursive flag
git clone --recursive https://github.com/nealmick/ned
cd ned
git submodule init
git submodule update

# macOS Intel/ARM)
brew install clang-format cmake llvm glfw glew pkg-config curl

# Ubuntu/Debian
sudo apt install cmake libglfw3-dev libglew-dev libgtk-3-dev pkg-config clang libcurl4-openssl-dev clang-format mesa-utils

# For Windows, the dependencies are installed using the build script
```

## Building the Project


### MacOS and Linux (ubuntu/debian)
```sh
./build.sh
```

### Windows
```sh
./build-win.bat
# On Windows, the build script will attempt to install Visual Studio with Build Tools. 10-20 minutes.
# After VS has been installed, you must close and re-open PowerShell and run ./build-win.bat again.
# Subsequent rebuilds are much faster after the initial dependencies have been installed.
```

Create app package
```sh
./pack-mac.sh
./pack-deb.sh

# Bypass quarantine/translocation or you can sign it with your own apple dev acc
xattr -dr com.apple.quarantine Ned.app

```


# Embed Ned in Your Dear ImGui Projects

https://github.com/user-attachments/assets/56c17e13-729b-4667-a6d4-95119f059252
### [github.com/nealmick/ImGui_Ned_Embed](https://github.com/nealmick/ImGui_Ned_Embed)

Ned can be embedded in other ImGui applications, taking advantage of its text editor, file explorer, and terminal emulator. The embedded version also includes emoji support, themes, and much more. We have a demo repository that shows how to get started embedding the neditor into your projects.







# About the Project
Ned is a feature-rich text editor built with Dear ImGui that combines the power of modern development tools with a lightweight, embeddable architecture. At its core, Ned provides a sophisticated text editing experience with Tree Sitter syntax highlighting supporting over 15 programming languages including C++, Python, JavaScript, Rust, Go, and more. The editor features custom lexer modes for specialized file types and includes advanced features like multi-cursor editing, line jumping, and a built-in file tree explorer.

The editor includes LSP integration with support for clangd, gopls, pyright, and TypeScript language servers, providing goto definition, find references, and symbol information. Ned also includes a terminal emulator and AI integration with OpenRouter support. The editor features emoji support with proper font rendering, custom shader effects, and a theming system. The project is designed to be embeddable in other ImGui applications through the ned_embed library, making it easy to integrate into your own projects.

Currently Ned is tested on macOS ARM and Intel, Windows x64, and has a Debian build available. Windows support includes automated dependency management through the build script.  

If you have questions or issues, feel free to reach out.


# 👷 Work In Progress 🔨
#### MCP Agent 
Ned has an AI agent that uses OpenRouter to connect to the latest models. The agent can use MCP to call tools such as read file, run command, or edit file. The edit file tool uses a specialized model called Morph to apply code edits on large files at high speed with high accuracy, similar to Cursor. Check it out at [morph.so](https://morphllm.com). The whole system is tied into the settings where the key for the agent and completion model is stored. Below is a demo of the agent:

https://github.com/user-attachments/assets/13c01a86-3b16-49c8-89e8-3fb5d7fb8910

#### Multi Cursor
Ned has the ability to track multiple cursors at once, which can make editing in certain scenarios much easier. The multi cursor system is used for file content searches to spawn cursors at each instance of a text search string. The app also supports multi selection for selecting text with multiple cursors. The cursor also supports keybinds such as jump to line end or jump one word forward. Below is a demo:

https://github.com/user-attachments/assets/b6537f42-fe11-4e5c-bd97-41f2db7bc262

#### Windows
Windows support is still being tested, but there is a windows build available in releases as well as a build script for both the standalone and embedded versions. 

https://github.com/user-attachments/assets/b3055f15-5180-4f44-b534-a9a75219ecf8


