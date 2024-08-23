# Text Editor written in C++
NED is a lightweight, feature-rich text editor built with C++ and ImGui. It offers syntax highlighting, project file tree, and a customizable interface.


<img src="https://i.imgur.com/Sb9geeH.png" alt="Ned Logo" width="100%" style="display: block; margin: auto; border-radius: 10px;">

#### Features
- Resizable panes
- Bookmarks
- Custom themes with JSON settings file
- Find and Replace
- File Tree Explorer
- Line numbering
- Syntax highlighting
- Rainbow cursor
- File type icons

# Getting Started
Prerequisites
CMake (version 3.10 or higher)
C++17 compatible compiler
OpenGL
GLFW3


Clone the repository with its submodules:
```sh
git clone --recursive https://github.com/yourusername/ned.git
cd ned
git submodule init
git submodule update

```

Building the Project
```sh
mkdir build
cd build
cmake ..

make

./text_editor
```


Configuration
NED uses a JSON configuration file in your home folder ~/.ned.json You can modify this file to change themes, font sizes, and other editor preferences.



Contributions are welcome! 

TODO:
 - Tokenizer: add more langues currently only python is supported, all other languages are  highligted single character at a time.  
 - Lexer:  add more languages currently only python is supported, all other languages use basic regex patterns to detirmine syntax colors.
 - Multi-cursor:  add keybind to create cursor at end of all find selection
 - Selection keybinds: currently cmd-a selects full file, but cmd-shift-a would be nice to select current indentation or current {} [] () "" '' `` blocks...
 - Jump to function definition with cmd click.
 - shift click select from old cursor positon to click position.
