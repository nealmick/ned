
<img src="https://github.com/nealmick/ned/actions/workflows/pack-app.yml/badge.svg" alt="Build Status">  ![macOS](https://img.shields.io/badge/macOS-✓-success?logo=apple&logoColor=white)  ![Debian](https://img.shields.io/badge/Debian-✓-success?logo=debian&logoColor=white)  ![Windows](https://img.shields.io/badge/Windows-✓-success?logo=microsoft&logoColor=white)

A retro-style text editor with GL shader effects. NED offers Tree-sitter syntax highlighting, LSP integration, git line markers, and a built-in terminal.


# [Download](https://github.com/nealmick/ned/releases)

https://github.com/user-attachments/assets/74af4120-7cf7-4e8c-9b60-7e2aa3228be0

## Shader Effects: Static Noise, Burn In, Screen Curvature, Bloom, Vignetting
| Amber | Solarized |
|---------|---------|
| <video src="https://github.com/user-attachments/assets/1461119f-8eef-4fe0-8564-c49ab1d0b227" width="300"></video> | <video src="https://github.com/user-attachments/assets/465204eb-cd81-4621-8a03-e5319b8a9103" width="300"></video> |

| Ned | Custom |
|---------|---------|
| <video src="https://github.com/user-attachments/assets/9f352517-2c51-4fa2-a008-84c254175326" width="300"></video> | <video src="https://github.com/user-attachments/assets/86c6810e-2507-440c-80cd-467df04483ce" width="300"></video> |



#### Notable Features
- OpenGL CRT-style shaders (noise, burn-in, curvature, bloom, scanlines, jitter, and more)
- Tree-sitter syntax highlighting for 15+ languages (C/C++, JS/TS/TSX, Python, Rust, Go, Java, C#, Ruby, Kotlin, HTML/CSS, JSON, TOML, HCL, Bash, …)
- LSP integration (goto definition, find references, symbol info) with configurable language servers
- Git changed-line markers in the gutter via libgit2
- Fullscreen terminal (ImGui-Terminal) toggled from the editor
- File tree explorer, fuzzy file finder, in-buffer find, and go-to-line
- Rainbow cursor and line numbers so the cursor is hard to lose
- Theme profiles (Ned, Amber, Solarized, custom) with live settings
- Embeddable `ned_embed` library for Dear ImGui hosts (multi-tab docking)



## Build from source
#### Prerequisites
- CMake 3.10 or higher
- C++20 compatible compiler
- OpenGL, GLFW3, GLEW
- FreeType
- Curl

Clone the repository with its submodules:
```sh
# Make sure you clone with the recursive flag
git clone --recursive https://github.com/nealmick/ned
cd ned
git submodule init
git submodule update

# macOS (Intel/ARM)
brew install clang-format cmake llvm glfw glew pkg-config curl freetype

# Ubuntu/Debian
sudo apt install cmake libglfw3-dev libglew-dev libgtk-3-dev pkg-config clang libcurl4-openssl-dev libfreetype6-dev clang-format mesa-utils

# For Windows, the dependencies are installed using the build script
```

## Building the Project


### macOS and Linux (Ubuntu/Debian)
```sh
./scripts/build.sh
```

### Windows
```sh
./scripts/build-win.bat
# On Windows, the build script will attempt to install Visual Studio with Build Tools. 10–20 minutes.
# After VS has been installed, you must close and re-open PowerShell and run ./scripts/build-win.bat again.
# Subsequent rebuilds are much faster after the initial dependencies have been installed.
```

### Tests
```sh
./scripts/test.sh
# Catch2 suites cover the document model (Monaco-shaped ports), commands, undo, save, git, and UTF-8 helpers.
```

Create app package
```sh
./scripts/pack-mac.sh
./scripts/pack-deb.sh

# Bypass quarantine/translocation, or sign with your own Apple developer account
xattr -dr com.apple.quarantine Ned.app
```


# Embed Ned in Your Dear ImGui Projects

https://github.com/user-attachments/assets/56c17e13-729b-4667-a6d4-95119f059252
### [github.com/nealmick/ImGui_Ned_Embed](https://github.com/nealmick/ImGui_Ned_Embed)

Ned can be embedded in other ImGui applications through the `ned_embed` library. The embed host adds multi-tab docking on top of the same editor core used by the standalone app — file explorer, terminal, themes, emoji fonts, and LSP are available to the host. The demo repository shows how to wire it into a project.



# About the Project
Ned is a Dear ImGui text editor aimed at a lightweight, embeddable core with a strong retro aesthetic. The editor is built around a document model and `EditorApi`: file I/O, LSP, and the shell talk to the editor through one interface, while highlighting, undo, save, and git live as focused services behind it.

Syntax highlighting uses Tree-sitter with query files for the languages above. LSP is driven by a configurable `lsp.json` (clangd, gopls, pyright/typescript-language-server, rust-analyzer, and others). The terminal is a fullscreen overlay powered by [ImGui-Terminal](https://github.com/nealmick/ImGui-Terminal). Shader effects, theme profiles, and keybinds live under user config (`~/ned/config`) seeded from bundled resources.

Standalone Ned is single-buffer; multi-tab editing is provided by `NedEmbed` for host applications. Platforms tested: macOS ARM and Intel, Windows x64, and Debian. Windows builds use the automated dependency path in the build script.

If you have questions or issues, feel free to reach out.

