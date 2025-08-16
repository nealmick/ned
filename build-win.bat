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

REM Try to find vcpkg toolchain file
set VCPKG_TOOLCHAIN_FILE=
if exist "%CD%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set VCPKG_TOOLCHAIN_FILE=%CD%\vcpkg\scripts\buildsystems\vcpkg.cmake
) else if exist "%CD%\..\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set VCPKG_TOOLCHAIN_FILE=%CD%\..\vcpkg\scripts\buildsystems\vcpkg.cmake
) else if defined VCPKG_ROOT (
    set VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else (
    echo Warning: vcpkg toolchain file not found!
    echo Please set VCPKG_ROOT environment variable or ensure vcpkg is in the project directory
    echo Attempting build without vcpkg...
)

REM Configure with CMake using vcpkg toolchain if available
echo Configuring with CMake...
if defined VCPKG_TOOLCHAIN_FILE (
    echo Using vcpkg toolchain: %VCPKG_TOOLCHAIN_FILE%
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows
) else (
    echo Building without vcpkg toolchain
    cmake .. -G "Visual Studio 17 2022" -A x64
)

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