# Ned
A retro-style text editor with GL shader effects. NED offers Tree Sitter syntax highlighting, LSP integration, and a terminal emulator.


# [Download](https://github.com/nealmick/ned/releases)

https://github.com/user-attachments/assets/74af4120-7cf7-4e8c-9b60-7e2aa3228be0

## Shader Effects:  Static Noise, Burn In, Screen Curviture, Bloom, Vignetting. 
| Amber | Solarized |
|---------|---------|
| <video src="https://github.com/user-attachments/assets/1461119f-8eef-4fe0-8564-c49ab1d0b227" width="300"></video> | <video src="https://github.com/user-attachments/assets/465204eb-cd81-4621-8a03-e5319b8a9103" width="300"></video> |

| Ned | Custom |
|---------|---------|
| <video src="https://github.com/user-attachments/assets/9f352517-2c51-4fa2-a008-84c254175326" width="300"></video> | <video src="https://github.com/user-attachments/assets/86c6810e-2507-440c-80cd-467df04483ce" width="300"></video> |




#### Notable Features
- OpenGL Shaders with retro style for the best coding vibes
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
