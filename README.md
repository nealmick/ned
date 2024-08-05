# Text Editor written in C++
NED is a lightweight, feature-rich text editor built with C++ and ImGui. It offers syntax highlighting, file management, and a customizable interface.
#### Features
 - Syntax highlighting for multiple programming languages
 - File explorer with intuitive navigation
 - Customizable themes
 - Undo/Redo functionality
 - Find and Replace
 - Line numbering

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