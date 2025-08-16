@echo off
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

REM Bundle DLLs for distribution
echo Bundling required DLLs...
if not exist "Release\dlls" mkdir "Release\dlls"

REM Copy all required DLLs from vcpkg
copy "vcpkg_installed\x64-windows\bin\*.dll" "Release\" >nul 2>&1
if %errorlevel% neq 0 (
    echo Warning: Some DLLs may not have been copied
)

REM Create a distributable package
echo Creating distributable package...
if not exist "ned-windows-portable" mkdir "ned-windows-portable"

REM Copy executable and DLLs
copy "Release\ned.exe" "ned-windows-portable\" >nul
copy "Release\*.dll" "ned-windows-portable\" >nul 2>&1

REM Copy Visual C++ Runtime DLLs (required for fresh Windows installs)
set "VC_REDIST_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC"
if not exist "%VC_REDIST_PATH%" set "VC_REDIST_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC"
if not exist "%VC_REDIST_PATH%" set "VC_REDIST_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC"
for /d %%i in ("%VC_REDIST_PATH%\*") do (
    if exist "%%i\x64\Microsoft.VC143.CRT\*.dll" (
        copy "%%i\x64\Microsoft.VC143.CRT\*.dll" "ned-windows-portable\" >nul 2>&1
        echo Copied VC++ Runtime DLLs from %%i
    )
)

REM Copy resource directories
echo Copying resource directories...
xcopy "..\fonts" "ned-windows-portable\fonts\" /E /I /Q
xcopy "..\icons" "ned-windows-portable\icons\" /E /I /Q
xcopy "..\settings" "ned-windows-portable\settings\" /E /I /Q
xcopy "..\shaders" "ned-windows-portable\shaders\" /E /I /Q
xcopy "..\editor\queries" "ned-windows-portable\queries\" /E /I /Q

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