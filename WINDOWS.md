# Windows Build Guide

This guide walks you through building ned on Windows from scratch, including all dependencies and toolchain setup.

## Prerequisites

### Required Software

1. **Visual Studio 2022** (Community Edition or higher)
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Required workloads:
     - Desktop development with C++
     - Windows 10/11 SDK

2. **Git for Windows**
   - Download from: https://git-scm.com/download/win
   - Ensure Git is added to your PATH

3. **PowerShell** (usually pre-installed on Windows 10/11)

## Setup Instructions

### 1. Install vcpkg Package Manager

vcpkg is Microsoft's C++ package manager that handles all our dependencies.

```powershell
# Clone vcpkg to C:\Users\[username]\source\
cd C:\Users\$env:USERNAME\source
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Add vcpkg to your PATH environment variable
$env:PATH += ";C:\Users\$env:USERNAME\source\vcpkg"
```

**Important**: Add vcpkg to your permanent PATH:
1. Open "Environment Variables" in Windows Settings
2. Add `C:\Users\[your-username]\source\vcpkg` to your user PATH
3. Restart PowerShell to verify: `vcpkg version`

### 2. Clone and Setup ned

```powershell
# Clone the ned repository
cd C:\Users\$env:USERNAME\source
git clone https://github.com/nealmick/ned.git
cd ned

# Switch to the windows branch
git checkout windows
```

### 3. Install Dependencies

The project uses a `vcpkg.json` manifest file to automatically install dependencies:

```powershell
# Install all required packages (this may take several minutes)
vcpkg install
```

The following packages will be installed:
- **glfw3**: OpenGL window management
- **glew**: OpenGL extension loading
- **curl**: HTTP client library
- **freetype**: Font rendering
- **opengl**: Graphics API

### 4. Build ned

```powershell
# Run the optimized Windows build script
.\build-win.bat
```

**Build Script Features**:
- Uses all CPU cores for parallel compilation
- Automatically configures CMake with vcpkg integration
- Creates optimized Release build
- Clean build option: `.\build-win.bat clean`

### 5. Run ned

After successful build:

```powershell
# Run from the build directory (where resources are located)
cd build
.\Release\ned.exe
```

You should see the ned welcome screen! 🎉

## Troubleshooting

### Common Issues

**"vcpkg not found"**
- Ensure vcpkg is in your PATH
- Restart PowerShell after adding to PATH
- Verify with: `vcpkg version`

**"CMake configuration failed"**
- Ensure Visual Studio 2022 is properly installed
- Verify Windows SDK is installed
- Try a clean build: `.\build-win.bat clean`

**"Cannot find glfw3" or similar package errors**
- Run `vcpkg install` again
- Check that vcpkg packages are in `vcpkg_installed/x64-windows/`

**Build is very slow**
- The first build compiles all dependencies and may take 5-10 minutes
- Subsequent builds should be much faster
- Ensure you're using the parallel build script

### Performance Notes

- **First Build**: 5-10 minutes (compiles all dependencies)
- **Incremental Builds**: 30 seconds - 2 minutes
- **Clean Builds**: 3-5 minutes (dependencies are cached)

The build script uses all available CPU cores (`%NUMBER_OF_PROCESSORS%`) for optimal performance.

## What's Working

✅ **Full Application Launch**
- ned.exe builds and runs successfully
- Welcome screen displays properly
- Settings system initializes
- All core UI components load

✅ **Graphics and Rendering**
- OpenGL initialization
- ImGui interface rendering
- Font loading and display
- Icon and shader systems

✅ **File System**
- File tree navigation
- Settings file management
- Resource loading from build directory

## Windows-Specific Notes

### Disabled Features

Some Unix-specific features are disabled on Windows but won't cause crashes:

- **Terminal Integration**: Unix terminal functionality is stubbed out
- **LSP (Language Server Protocol)**: Complex Unix process management is disabled
- **SSH Git Operations**: Some advanced Git features may be limited

These limitations don't affect core editing functionality.

### Platform Differences

- **Line Endings**: Git automatically handles CRLF conversion
- **File Paths**: All file path handling is Windows-compatible
- **System Integration**: Uses Windows-native file dialogs and system APIs

## Development Tips

### Faster Development Builds

```powershell
# For development, you can build specific targets
cd build
cmake --build . --config Release --target ned
```

### Debugging

To build with debug symbols:

```powershell
# Modify build-win.bat temporarily, change:
# cmake --build . --config Release
# to:
# cmake --build . --config Debug
```

### Clean Rebuilds

```powershell
# Full clean rebuild (removes build directory)
.\build-win.bat clean

# Just clean build cache (faster)
cd build
cmake --build . --target clean
```

## Architecture Overview

The Windows build uses:

- **CMake**: Cross-platform build system generator
- **Visual Studio 2022**: Native Windows compiler and toolchain
- **vcpkg**: Dependency management for C++ libraries
- **MSBuild**: Microsoft's build engine (called by CMake)

## Support

If you encounter issues:

1. Check this troubleshooting section
2. Ensure all prerequisites are properly installed
3. Try a clean build
4. Verify your PATH environment variables

## Contributing

When making changes to the Windows build:

1. Test both clean and incremental builds
2. Verify vcpkg.json includes all new dependencies
3. Update this guide if adding new requirements
4. Test on a clean Windows environment if possible

The Windows port maintains full compatibility with the existing codebase while providing a native Windows development experience.