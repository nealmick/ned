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



### MacOS Install
You can install Ned using Homebrew:

```bash
git clone --recursive https://github.com/nealmick/ned
cd ned
# Add the tap and install Ned
brew tap nealmick/tap
brew install ned

# Run the editor
ned
# note you must copy the .ned.json settings file into your homefolder ~
```


### Ubuntu and Windows install via WSL
You can install Ned using Ubuntu

```bash
git clone --recursive https://github.com/yourusername/ned.git
cd ned
sudo apt install -y build-essential libgl1-mesa-dev xorg-dev libglfw3 libglfw3-dev libglew-dev 
mkdir build
cd build
cmake ..

make

./ned
```


# Build from source
#### Prerequisites
CMake (version 3.10 or higher)
C++17 compatible compiler
OpenGL
GLFW3

Clone the repository with its submodules:
```sh
git clone --recursive https://github.com/nealmick/ned
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

./ned
```


Contributions are welcome! 

TODO:
 - Multi-cursor:  add keybind to create cursor at end of all find selection
 - Selection keybinds: currently cmd-a selects full file, but cmd-shift-a would be nice to select current indentation or current {} [] () "" '' `` blocks...
 - Jump to function definition with cmd click.
