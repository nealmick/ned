@echo off
echo Building NED for Windows...

REM Check if Visual Studio is installed
if not exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    echo Installing Visual Studio Community 2022 with C++ Desktop Development...
    winget install Microsoft.VisualStudio.2022.Community --override "--wait --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --quiet"
    echo Visual Studio installation completed. Please restart your terminal and run this script again.
    pause
    exit /b 0
)

REM Setup Visual Studio Developer environment (equivalent to setup-msbuild action)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    echo Setting up Visual Studio environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
) else (
    echo VsDevCmd.bat not found. Visual Studio may not be fully installed.
    echo Please install Visual Studio Community 2022 with "Desktop development with C++" workload manually.
    pause
    exit /b 1
)

REM Everything below is identical to build-win-ci.bat
set VCPKG_ROOT=%CD%\vcpkg
set VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM Install vcpkg if not present
if not exist "%VCPKG_ROOT%" (
    echo Installing vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    if %errorlevel% neq 0 (
        echo Failed to clone vcpkg!
        exit /b 1
    )
    
    cd vcpkg
    call bootstrap-vcpkg.bat
    if %errorlevel% neq 0 (
        echo Failed to bootstrap vcpkg!
        exit /b 1
    )
    cd ..
) else (
    echo vcpkg already exists, skipping installation
)

REM Install dependencies via vcpkg
echo Installing dependencies via vcpkg...
%VCPKG_ROOT%\vcpkg.exe install --triplet x64-windows
if %errorlevel% neq 0 (
    echo Failed to install vcpkg dependencies!
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

cd build

REM Configure with CMake using vcpkg toolchain
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows

REM Check if configuration succeeded
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project with parallel compilation using all CPU cores
echo Building project using %NUMBER_OF_PROCESSORS% CPU cores...
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%

REM Check if build succeeded
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable should be in build\Release\ned.exe

REM Run the built executable
echo Starting ned.exe...
.\Release\ned.exe

pause