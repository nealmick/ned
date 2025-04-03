# A Simple Text Editor
NED is a lightweight, feature-rich text editor built with C++ and ImGui. It offers syntax highlighting, LSP Support, Project file tree, and a customizable interface.


<img src="https://i.imgur.com/Sb9geeH.png" alt="Ned Logo" width="100%" style="display: block; margin: auto; border-radius: 10px;">

#### Noteable Features
- Text Bookmarks
- Rainbow mode
- OpenGL Shaders
- LSP Adapters
- Terminal Emulator
- Resizable panes
- Custom themes with JSON settings file
- File Tree Explorer
- Custom lexers and tokenizers
- File type icons


# Build from source
#### Prerequisites
CMake (version 3.10 or higher)
C++17 compatible compiler
OpenGL
GLFW3
Glew

Clone the repository with its submodules:
```sh
git clone --recursive https://github.com/nealmick/ned
cd ned
git submodule init
git submodule update

#Mac OS Intel and ARM
brew install clang-format cmake llvm glfw glew pkg-config

#Ubuntu may require patching logic
sudo apt install cmake libglfw3-dev libglew-dev libgtk-3-dev pkg-config build-essential
```

Building the Project
```sh
./build.sh

```

Create app package
```sh
./pack.sh

# Bypass quarantine/translocation or you can sign it with your own apple dev acc
xattr -dr com.apple.quarantine Ned.app

```



Contributions are welcome! 

TODO:
 - Multi-cursor:  add keybind to create cursor at end of all find selection
 - Selection keybinds: currently cmd-a selects full file, but cmd-shift-a would be nice to select current indentation or current {} [] () "" '' `` blocks...
 - Jump to function definition with cmd click.
 - Add more lexers, the JSX lexer need to be fixed, next to add: GO,Java, and C#
 - fix many bugs...


