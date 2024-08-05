# Text Editor written in C++
NED is a lightweight, feature-rich text editor built with C++ and ImGui. It offers syntax highlighting, file management, and a customizable interface.


<img src="https://i.imgur.com/Sb9geeH.png" alt="Ned Logo" width="100%" style="display: block; margin: auto; border-radius: 10px;">

#### Features
- File Tree Explorer
- Resizable panes
- Custom themes with JSON settings file
- Find and Replace
- Line numbering
- Syntax highlighting for Python, C++, JavaScript, and more
- Rainbow cursor and line numbers
- File type icons

# Getting Started
Prerequisites
CMake (version 3.10 or higher)
C++17 compatible compiler
OpenGL
GLFW3

Cloning the Repository

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
NED uses a JSON configuration file located at ~/.ned.json You can modify this file to change themes, font sizes, and other editor preferences.


Contributing
Contributions are welcome! 

TODO:
 - Tokenizer to reduce the number of syntax operations by grouping together
 - Write lexer to replace regex syntax logic
 - Multi-cursor