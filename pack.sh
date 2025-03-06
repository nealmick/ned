# In pack.sh, adjust the code to properly handle library dependencies:

echo "Copying libraries..."
mkdir -p "$FRAMEWORKS"

# Function to safely copy libraries with error handling
copy_lib() {
    local source="$1"
    local dest="$2"
    
    if [ -f "$source" ]; then
        cp "$source" "$dest"
        echo "✅ Copied: $source"
    else
        echo "⚠️  Warning: Could not find $source"
        # Try alternative locations
        local lib_name=$(basename "$source")
        local alt_paths=(
            "$HOMEBREW_PREFIX/lib/$lib_name"
            "/usr/local/lib/$lib_name"
            "/opt/homebrew/lib/$lib_name"
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

# GLEW Library
GLEW_LIB="$HOMEBREW_PREFIX/Cellar/glew/2.2.0_1/lib/libGLEW.dylib"
copy_lib "$GLEW_LIB" "$FRAMEWORKS/"

# GLFW Library
GLFW_LIB="$HOMEBREW_PREFIX/Cellar/glfw/3.4/lib/libglfw.3.dylib"
if [ ! -f "$GLFW_LIB" ]; then
    GLFW_LIB="$HOMEBREW_PREFIX/Cellar/glfw/3.3.9/lib/libglfw.3.dylib"
fi
copy_lib "$GLFW_LIB" "$FRAMEWORKS/"

echo "Fixing library paths..."
# Make sure to sign each library
for lib in "$FRAMEWORKS"/*.dylib; do
    if [ -f "$lib" ]; then
        codesign --force --sign - "$lib"
    fi
done

# Use install_name_tool to update dynamic library paths in the executable
install_name_tool -change "@rpath/libGLEW.dylib" "@executable_path/../Frameworks/libGLEW.dylib" "$MACOS/$APP_NAME" 2>/dev/null || true
install_name_tool -change "@rpath/libglfw.3.dylib" "@executable_path/../Frameworks/libglfw.3.dylib" "$MACOS/$APP_NAME" 2>/dev/null || true
install_name_tool -change "/opt/homebrew/opt/glew/lib/libGLEW.dylib" "@executable_path/../Frameworks/libGLEW.dylib" "$MACOS/$APP_NAME" 2>/dev/null || true
install_name_tool -change "/opt/homebrew/opt/glfw/lib/libglfw.3.dylib" "@executable_path/../Frameworks/libglfw.3.dylib" "$MACOS/$APP_NAME" 2>/dev/null || true
install_name_tool -change "/usr/local/opt/glew/lib/libGLEW.dylib" "@executable_path/../Frameworks/libGLEW.dylib" "$MACOS/$APP_NAME" 2>/dev/null || true
install_name_tool -change "/usr/local/opt/glfw/lib/libglfw.3.dylib" "@executable_path/../Frameworks/libglfw.3.dylib" "$MACOS/$APP_NAME" 2>/dev/null || true

# Sign the executable and the whole app
codesign --force --sign - "$MACOS/$APP_NAME"
codesign --force --deep --sign - "$APP_BUNDLE"