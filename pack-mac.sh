#!/bin/bash
# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Detect architecture and set proper paths
if [[ $(uname -m) == 'arm64' ]]; then
    # Apple Silicon
    HOMEBREW_PREFIX="/opt/homebrew"
else
    # Intel
    HOMEBREW_PREFIX="/usr/local"
fi

echo -e "${BLUE}📦 Creating macOS app bundle...${NC}"

# Check if we're building with the right deployment target
if [[ "$OSTYPE" == "darwin"* ]]; then
    export MACOSX_DEPLOYMENT_TARGET=11.0
    echo -e "${BLUE}🍎 Setting macOS deployment target to 11.0${NC}"
fi

# Ensure the app exists
if [ ! -f ".build/ned" ]; then
    echo -e "${RED}❌ Application not found. Run build.sh first.${NC}"
    exit 1
fi

# App bundle structure
APP_NAME="Ned"
APP_BUNDLE="$APP_NAME.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"
FRAMEWORKS="$CONTENTS/Frameworks"

# Create directory structure
mkdir -p "$MACOS"
mkdir -p "$RESOURCES"
mkdir -p "$FRAMEWORKS"

echo "Copying resources..."
cp .build/ned "$MACOS/$APP_NAME"
cp -r fonts "$RESOURCES/"
cp -r icons "$RESOURCES/"
cp -r shaders "$RESOURCES/"
cp -r settings "$RESOURCES/"
cp -r editor/queries "$RESOURCES/" 


echo "Adding app icon..."
cp -r icons/ned.icns "$RESOURCES/ned.icns"

# Create Info.plist
cat > "$CONTENTS/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIconFile</key>
    <string>ned.icns</string>
    <key>CFBundleIdentifier</key>
    <string>com.nealaggarwal.ned</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# Copy required libraries - with error handling
echo "Copying libraries..."

copy_lib() {
    local source="$1"
    local dest="$2"
    
    if [ -f "$source" ]; then
        cp "$source" "$dest"
        echo "✅ Copied: $source"
    else
        echo "⚠️  Warning: Could not find $source"
        local lib_name
        lib_name=$(basename "$source")
        local alt_paths=(
            "$HOMEBREW_PREFIX/lib/$lib_name"
            "/usr/local/lib/$lib_name"
            "/usr/lib/$lib_name"
        )
        for alt_path in "${alt_paths[@]}"; do
            if [ -f "$alt_path" ]; then
                cp "$alt_path" "$dest"
                echo "✅ Found alternative: $alt_path"
                return 0
            fi
        done
        echo "❌ Could not find $lib_name in any standard location"
    fi
}

# GLEW Library - find the correct versioned library
GLEW_LIBS=(
    "$HOMEBREW_PREFIX/lib/libGLEW.2.2.dylib"
    "$HOMEBREW_PREFIX/Cellar/glew/*/lib/libGLEW.2.2.dylib"
    "$HOMEBREW_PREFIX/opt/glew/lib/libGLEW.2.2.dylib"
)

for glew_path in "${GLEW_LIBS[@]}"; do
    if [ -f $glew_path ]; then
        copy_lib "$glew_path" "$FRAMEWORKS/"
        break
    fi
done

# GLFW Library
GLFW_LIBS=(
    "$HOMEBREW_PREFIX/lib/libglfw.3.dylib"
    "$HOMEBREW_PREFIX/Cellar/glfw/*/lib/libglfw.3.dylib"
    "$HOMEBREW_PREFIX/opt/glfw/lib/libglfw.3.dylib"
)

for glfw_path in "${GLFW_LIBS[@]}"; do
    if [ -f $glfw_path ]; then
        copy_lib "$glfw_path" "$FRAMEWORKS/"
        break
    fi
done

# FreeType Library
FREETYPE_LIBS=(
    "$HOMEBREW_PREFIX/lib/libfreetype.6.dylib"
    "$HOMEBREW_PREFIX/Cellar/freetype/*/lib/libfreetype.6.dylib"
    "$HOMEBREW_PREFIX/opt/freetype/lib/libfreetype.6.dylib"
)

for freetype_path in "${FREETYPE_LIBS[@]}"; do
    if [ -f $freetype_path ]; then
        copy_lib "$freetype_path" "$FRAMEWORKS/"
        break
    fi
done

# PNG Library (needed by FreeType)
PNG_LIBS=(
    "$HOMEBREW_PREFIX/lib/libpng16.16.dylib"
    "$HOMEBREW_PREFIX/Cellar/libpng/*/lib/libpng16.16.dylib"
    "$HOMEBREW_PREFIX/opt/libpng/lib/libpng16.16.dylib"
)

for png_path in "${PNG_LIBS[@]}"; do
    if [ -f $png_path ]; then
        copy_lib "$png_path" "$FRAMEWORKS/"
        break
    fi
done

echo "Fixing library paths..."

# Get the actual library paths from the binary
CURRENT_GLEW=$(otool -L "$MACOS/$APP_NAME" | grep GLEW | awk '{print $1}' | head -1)
CURRENT_GLFW=$(otool -L "$MACOS/$APP_NAME" | grep glfw | awk '{print $1}' | head -1)

if [ -n "$CURRENT_GLEW" ]; then
    install_name_tool -change "$CURRENT_GLEW" "@executable_path/../Frameworks/libGLEW.2.2.dylib" "$MACOS/$APP_NAME"
    echo "Fixed GLEW path: $CURRENT_GLEW -> @executable_path/../Frameworks/libGLEW.2.2.dylib"
fi

if [ -n "$CURRENT_GLFW" ]; then
    install_name_tool -change "$CURRENT_GLFW" "@executable_path/../Frameworks/libglfw.3.dylib" "$MACOS/$APP_NAME"
    echo "Fixed GLFW path: $CURRENT_GLFW -> @executable_path/../Frameworks/libglfw.3.dylib"
fi

# Get FreeType path from binary
CURRENT_FREETYPE=$(otool -L "$MACOS/$APP_NAME" | grep freetype | awk '{print $1}' | head -1)

if [ -n "$CURRENT_FREETYPE" ]; then
    install_name_tool -change "$CURRENT_FREETYPE" "@executable_path/../Frameworks/libfreetype.6.dylib" "$MACOS/$APP_NAME"
    echo "Fixed FreeType path: $CURRENT_FREETYPE -> @executable_path/../Frameworks/libfreetype.6.dylib"
fi

# Fix dependencies within the bundled libraries themselves
echo "Fixing internal library dependencies..."

# Fix dependencies within the bundled libraries themselves
for lib in "$FRAMEWORKS"/*.dylib; do
    echo "Checking dependencies for $(basename "$lib")..."
    
    # Get all homebrew/usr/local dependencies for this library
    LIB_DEPS=$(otool -L "$lib" | grep -E "(homebrew|usr/local)" | awk '{print $1}')
    
    for dep_path in $LIB_DEPS; do
        dep_name=$(basename "$dep_path")
        
        # If we have this dependency in our Frameworks, fix the path
        if [ -f "$FRAMEWORKS/$dep_name" ]; then
            install_name_tool -change "$dep_path" "@loader_path/$dep_name" "$lib"
            echo "Fixed internal dependency: $dep_path -> @loader_path/$dep_name in $(basename "$lib")"
        else
            echo "⚠️  Missing dependency for $(basename "$lib"): $dep_name"
            # Try to copy the missing dependency
            copy_lib "$dep_path" "$FRAMEWORKS/"
            if [ -f "$FRAMEWORKS/$dep_name" ]; then
                install_name_tool -change "$dep_path" "@loader_path/$dep_name" "$lib"
                echo "Fixed newly copied dependency: $dep_path -> @loader_path/$dep_name"
            fi
        fi
    done
done

echo "Final check for any remaining missing libraries..."
MISSING_LIBS=$(otool -L "$MACOS/$APP_NAME" | grep -E "(homebrew|usr/local)" | awk '{print $1}')

for lib_path in $MISSING_LIBS; do
    lib_name=$(basename "$lib_path")
    if [ ! -f "$FRAMEWORKS/$lib_name" ]; then
        echo "⚠️  Still missing: $lib_name"
        copy_lib "$lib_path" "$FRAMEWORKS/"
        
        if [ -f "$FRAMEWORKS/$lib_name" ]; then
            install_name_tool -change "$lib_path" "@executable_path/../Frameworks/$lib_name" "$MACOS/$APP_NAME"
            echo "Fixed main app dependency: $lib_path -> @executable_path/../Frameworks/$lib_name"
        fi
    fi
done

# Sign the libs
echo "Signing libraries..."
for lib in "$FRAMEWORKS"/*.dylib; do
    codesign --force --sign - "$lib"
done

# Sign the main executable
echo "Signing executable..."
codesign --force --sign - "$MACOS/$APP_NAME"

# Sign the entire .app
echo "Signing app bundle..."
codesign --force --deep --sign - "$APP_BUNDLE"

# Verify the app bundle structure
echo "Verifying app bundle..."
echo "Minimum system version: $(plutil -p "$CONTENTS/Info.plist" | grep LSMinimumSystemVersion)"
echo "Architectures: $(file "$MACOS/$APP_NAME" | grep -o 'x86_64\|arm64' | tr '\n' ' ')"

# Instead of creating a DMG, create a ZIP
echo "Creating ZIP archive of the .app..."
ZIP_NAME="$APP_NAME.zip"
zip -r "$ZIP_NAME" "$APP_BUNDLE"

echo -e "${GREEN}✅ Package created: $ZIP_NAME${NC}"