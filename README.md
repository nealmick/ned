Ned
A retro-style text editor with GL shader effects. NED offers Tree Sitter syntax highlighting, LSP integration, and a terminal emulator.

https://github.com/user-attachments/assets/d4358284-0fd3-41de-8f78-74172c7f3349

# [Releases](https://github.com/nealmick/ned/releases)

#### Notable Features
- OpenGL Shaders with retro style effects, burn-in, static noise, pixelation
- Text Bookmarks make editing multiple files with saved cursors a breeze
- Rainbow mode cursor to never lose your cursor, and stand out
- LSP Adapters for easy navigation and advanced language support
- Terminal Emulator based on suckless st.c ported to C++ with multiplexer support
- Optional Custom lexers and tokenizers for custom languages and obscure syntax patterns
- Copilot-like auto complete using open router, choose the latest and best LLM models
- Multi cursor support, easily find and replace strings with multi selection



## Build from source
#### Prerequisites
CMake (version 3.10 or higher)
C++17 compatible compiler
OpenGL
GLFW3
Glew
Curl

Clone the repository with its submodules:
```sh
git clone --recursive https://github.com/nealmick/ned
cd ned
git submodule init
git submodule update

#Mac OS Intel and ARM
brew install clang-format cmake llvm glfw glew pkg-config curl

#Ubuntu may require patching logic, currently un-tested
sudo apt install cmake libglfw3-dev libglew-dev libgtk-3-dev pkg-config build-essential libcurl4-openssl-dev clang-format mesa-utils

```

Building the Project
```sh
./build.sh

```

Create app package
```sh
./pack-mac.sh
./pack-deb.sh

# Bypass quarantine/translocation or you can sign it with your own apple dev acc
xattr -dr com.apple.quarantine Ned.app

```