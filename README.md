# Text Editor written in C++
NED is a lightweight, feature-rich text editor built with C++ and ImGui. It offers syntax highlighting, project file tree, and a customizable interface.


<img src="https://i.imgur.com/Sb9geeH.png" alt="Ned Logo" width="100%" style="display: block; margin: auto; border-radius: 10px;">

#### Features
- Resizable panes
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

Key Bindings:
CTRL and CMD are used interchangably depending on mac/windows keyboard layout

CMD F 
	- Open finder window
	- Enter/return - find next
	- shift enter/return - find previous
CMD V
	- Paste current clipboard content to cursor
CMD C
	- Copy cursor selection
CMD A
	- Select full file
CMD LEFT
	- Move cursor to line start
CMD RIGHT
	- Move cursor to line end
CMD UP
	- Move cursor up 6 lines
CMD DOWN
	- Move cursor down 6 lines
CMD Z
	- Undo last change
CMD SHIFT Z
	- Redo last change

 








