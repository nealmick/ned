@echo off
cd /d "%~dp0\.."
echo Building NED for Windows in CI environment...

REM Check if we're running in CI
if "%CI%"=="true" (
    echo Detected CI environment
) else (
    echo Running in local environment
)

REM Set up environment variables
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
%VCPKG_ROOT%\vcpkg.exe install --triplet x64-windows-static
if %errorlevel% neq 0 (
    echo Failed to install vcpkg dependencies!
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

REM A leftover x64-windows (dynamic) CMake cache points Fontconfig/ZLIB at
REM deleted headers and configure dies. Wipe just the cache, keep static libs.
if exist build\CMakeCache.txt (
    findstr /C:"vcpkg_installed/x64-windows/" /C:"vcpkg_installed\\x64-windows\\" build\CMakeCache.txt >nul
    if not errorlevel 1 (
        echo Clearing stale dynamic-vcpkg CMake cache...
        del /q build\CMakeCache.txt
        if exist build\CMakeFiles rmdir /s /q build\CMakeFiles
    )
)
if exist build\vcpkg_installed\x64-windows rmdir /s /q build\vcpkg_installed\x64-windows

cd build

REM Configure with CMake using vcpkg toolchain
REM NED_BUILD_TESTS=OFF for release package speed; CI runs ned_tests in a separate step.
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DNED_BUILD_TESTS=OFF

REM Check if configuration succeeded
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build the project with parallel compilation (suppress warnings for cleaner output)
echo Building project...
cmake --build . --config Release --parallel -- /p:WarningLevel=1

REM Check if build succeeded
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Executable should be in build\Release\ned.exe

REM Create a distributable package (statically linked — no sidecar DLLs)
echo Creating distributable package...
if exist "ned-windows-portable" rmdir /s /q "ned-windows-portable"
mkdir "ned-windows-portable"

copy "Release\ned.exe" "ned-windows-portable\" >nul
if %errorlevel% neq 0 (
    echo Failed to copy ned.exe
    exit /b 1
)

REM Runtime data the exe loads from disk (fonts, icons, configs, shaders, queries).
echo Copying resource directories...
xcopy "..\resources" "ned-windows-portable\resources\" /E /I /Q
mkdir "ned-windows-portable\shaders"
copy "..\shaders\vertex.glsl" "ned-windows-portable\shaders\" >nul
copy "..\shaders\fragment.glsl" "ned-windows-portable\shaders\" >nul
copy "..\shaders\burn_in.frag" "ned-windows-portable\shaders\" >nul
xcopy "..\editor\services\highlight\queries" "ned-windows-portable\queries\" /E /I /Q
if exist "rgb.txt" copy "rgb.txt" "ned-windows-portable\" >nul
if exist "..\lib\imgui-terminal\rgb.txt" copy "..\lib\imgui-terminal\rgb.txt" "ned-windows-portable\" >nul

REM fontconfig configs (FcInit). imgui-terminal looks next to the exe.
if exist "vcpkg_installed\x64-windows-static\etc\fonts" (
    xcopy "vcpkg_installed\x64-windows-static\etc\fonts" "ned-windows-portable\fontconfig\" /E /I /Q
) else if exist "..\vcpkg_installed\x64-windows-static\etc\fonts" (
    xcopy "..\vcpkg_installed\x64-windows-static\etc\fonts" "ned-windows-portable\fontconfig\" /E /I /Q
) else if exist "Release\fontconfig" (
    xcopy "Release\fontconfig" "ned-windows-portable\fontconfig\" /E /I /Q
)

REM Create clean folder structure for CI
if "%CI%"=="true" (
    echo Creating clean folder structure for CI...
    REM Create ned-release directory with ned folder inside
    cd ..
    if exist "ned-release" rmdir /s /q "ned-release"
    mkdir "ned-release"
    mkdir "ned-release\ned"
    xcopy "build\ned-windows-portable\*" "ned-release\ned\" /E /I /Q
    echo Folder 'ned-release/ned' created for GitHub Actions upload
)

echo Portable package created in ned-windows-portable\

REM For CI, don't run the executable
if "%CI%"=="true" (
    echo CI build complete - portable package ready
) else (
    echo Starting ned.exe...
    .\Release\ned.exe
    pause
)