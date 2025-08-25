#!/bin/bash
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'


echo -e "${BLUE}📦 Creating Debian package...${NC}"

if ! command -v dpkg-deb &> /dev/null; then
    echo -e "${RED}❌ dpkg-deb not found. Install with: apt install dpkg-dev${NC}"
    exit 1
fi

BINARY_PATH=".build/ned"
if [ ! -f "$BINARY_PATH" ]; then
    echo -e "${RED}❌ Binary not found. Run build.sh first.${NC}"
    exit 1
fi

PKG_NAME="Ned"
VERSION="1.0.0"
ARCH=$(dpkg --print-architecture)
TEMP_DIR="deb-package"
INSTALL_DIR="$TEMP_DIR/usr"
LIB_DIR="$INSTALL_DIR/lib/$PKG_NAME"  # /usr/lib/Ned
BIN_DIR="$INSTALL_DIR/bin"
SHARE_DIR="$INSTALL_DIR/share/$PKG_NAME"  # /usr/share/Ned
DEBIAN_DIR="$TEMP_DIR/DEBIAN"

rm -rf "$TEMP_DIR"
mkdir -p "$LIB_DIR" "$BIN_DIR" "$SHARE_DIR" "$DEBIAN_DIR"

# =========================================================================
# Core Application Files
# =========================================================================
echo "Copying binary..."
install -Dm755 "$BINARY_PATH" "$LIB_DIR/ned"

echo "Creating wrapper script..."
cat > "$BIN_DIR/ned" << EOF
#!/bin/bash
exec /usr/lib/Ned/ned "\$@"
EOF
chmod 755 "$BIN_DIR/ned"

# =========================================================================
# Resource Files (Fonts/Queries/Icons/Shaders)
# =========================================================================
echo "Copying resources..."

# 1. Fonts
mkdir -p "$LIB_DIR/fonts"
cp -r fonts/* "$LIB_DIR/fonts/"

# 2. Queries
mkdir -p "$LIB_DIR/queries"
cp -r editor/queries/* "$LIB_DIR/queries/"

# 3. Other resources
cp -r icons "$SHARE_DIR/"
cp -r shaders "$SHARE_DIR/"
cp -r settings "$LIB_DIR/"

# =========================================================================
# Control File
# =========================================================================
echo "Creating control file..."
cat > "$DEBIAN_DIR/control" << EOF
Package: $PKG_NAME
Version: $VERSION
Architecture: $ARCH
Maintainer: Neal Mick <nealmick99@gmail.com>
Section: utils
Priority: optional
Depends: libglfw3, libglew2.2, libcurl4, libgtk-3-0
Homepage: https://github.com/nealmick/ned
Description: Modern code editor with AI integration
 Ned is a lightweight, cross-platform code editor
 and AI-powered features.
EOF

# =========================================================================
# Permissions
# =========================================================================
echo "Setting permissions..."
find "$TEMP_DIR" -type d -exec chmod 755 {} \;
find "$TEMP_DIR" -type f -exec chmod 644 {} \;
chmod 755 "$LIB_DIR/ned" "$BIN_DIR/ned"

# =========================================================================
# Build Package
# =========================================================================
DEB_FILE="Ned_${ARCH}.deb"
echo -e "${BLUE}📦 Building package...${NC}"
dpkg-deb --build "$TEMP_DIR" "$DEB_FILE" > /dev/null

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Package created: ${DEB_FILE}${NC}"
else
    echo -e "${RED}❌ Package creation failed!${NC}"
    exit 1
fi

# =========================================================================
# Verification
# =========================================================================
echo -e "${BLUE}Verifying installed files...${NC}"
echo "Fonts:"
dpkg -c "$DEB_FILE" | grep "fonts/.*\.ttf"

echo "Queries:"
dpkg -c "$DEB_FILE" | grep "queries/.*\.scm"

rm -rf "$TEMP_DIR"