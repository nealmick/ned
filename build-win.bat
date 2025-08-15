@echo off
echo Building NED for Windows...

REM Check if user wants a clean build
if "%1"=="clean" (
    echo Performing clean build...
    if exist build (
        rmdir /s /q build
    )
)

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

cd build

REM Configure with CMake using vcpkg toolchain
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="C:/Users/nealm/source/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows

REM Check if configuration succeeded
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project with parallel compilation
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