#!/bin/bash

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}📦 Creating macOS app bundle...${NC}"

# Check if app is built
if [ ! -f ".build/ned" ]; then
    echo -e "${RED}Error: Build the app first with ./build.sh${NC}"
    exit 1
fi

# Create app structure
rm -rf Ned.app
mkdir -p Ned.app/Contents/{MacOS,Resources,Frameworks}

# Copy executable
cp .build/ned Ned.app/Contents/MacOS/

# Copy resources - preserve directory structure!
echo -e "${BLUE}Copying resources...${NC}"
cp -R fonts Ned.app/Contents/Resources/
cp -R icons Ned.app/Contents/Resources/
cp -R shaders Ned.app/Contents/Resources/

# Also copy these resources to where the app will look for them
mkdir -p Ned.app/Contents/MacOS/fonts
mkdir -p Ned.app/Contents/MacOS/icons
mkdir -p Ned.app/Contents/MacOS/shaders
cp -R fonts/* Ned.app/Contents/MacOS/fonts/
cp -R icons/* Ned.app/Contents/MacOS/icons/
cp -R shaders/* Ned.app/Contents/MacOS/shaders/

# Handle app icon specifically
if [ -f "icons/ned.icns" ]; then
    echo -e "${BLUE}Adding app icon...${NC}"
    cp icons/ned.icns Ned.app/Contents/Resources/
else
    echo -e "${YELLOW}Warning: icons/ned.icns not found. Using default icon.${NC}"
fi

# Get the GLEW library path from CMakeLists.txt
GLEW_PATH=$(grep -o "/.*libGLEW.*dylib" CMakeLists.txt | head -1)
cp "$GLEW_PATH" Ned.app/Contents/Frameworks/

# Find GLFW library
GLFW_PATH=$(find /opt/homebrew /usr/local -name "libglfw*.dylib" | head -1)
cp "$GLFW_PATH" Ned.app/Contents/Frameworks/

# Create a minimal Info.plist
cat > Ned.app/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>ned</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourname.ned</string>
    <key>CFBundleName</key>
    <string>Ned</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleIconFile</key>
    <string>ned.icns</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF

# Fix library paths in the executable
GLEW_NAME=$(basename "$GLEW_PATH")
GLFW_NAME=$(basename "$GLFW_PATH")

echo -e "${BLUE}Fixing library paths...${NC}"
install_name_tool -change "$GLEW_PATH" "@executable_path/../Frameworks/$GLEW_NAME" Ned.app/Contents/MacOS/ned
install_name_tool -change "$GLFW_PATH" "@executable_path/../Frameworks/$GLFW_NAME" Ned.app/Contents/MacOS/ned

# Clear application icon cache
touch Ned.app

# Create DMG
echo -e "${BLUE}Creating DMG...${NC}"
hdiutil create -volname "Ned" -srcfolder Ned.app -ov -format UDZO Ned.dmg

echo -e "${GREEN}✅ Package created: Ned.dmg${NC}"