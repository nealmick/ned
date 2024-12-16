#!/bin/bash

# Get the directory where your ned project is
NED_PROJECT_DIR=~/ned  # Set this to where your project is

# Debug output
echo "Using project directory: $NED_PROJECT_DIR"

# Check if required directories exist
if [ ! -f "$NED_PROJECT_DIR/build/text_editor" ]; then
    echo "Error: Can't find text_editor binary. Make sure you've built the project."
    exit 1
fi

if [ ! -d "$NED_PROJECT_DIR/fonts" ]; then
    echo "Error: Can't find fonts directory"
    exit 1
fi

if [ ! -d "$NED_PROJECT_DIR/icons" ]; then
    echo "Error: Can't find icons directory"
    exit 1
fi

# Create packaging directory
PACKAGE_DIR=~/ned-package
rm -rf $PACKAGE_DIR
mkdir -p $PACKAGE_DIR
cd $PACKAGE_DIR

echo "Created package directory at: $PACKAGE_DIR"

# Create directory structure
mkdir -p ned-1.0.0/DEBIAN
mkdir -p ned-1.0.0/usr/bin
mkdir -p ned-1.0.0/usr/share/ned/fonts
mkdir -p ned-1.0.0/usr/share/ned/icons

echo "Created directory structure"

# Create control file
cat > ned-1.0.0/DEBIAN/control << EOL
Package: ned
Version: 1.0.0
Section: editors
Priority: optional
Architecture: amd64
Depends: libglfw3, libgtk-3-0
Maintainer: Neal Mick <your.email@example.com>
Description: Modern text editor built with ImGui
 A fast, modern text editor with syntax highlighting
 and various modern features.
EOL

echo "Created control file"

# Copy files - with debug output
echo "Copying binary..."
cp -v "$NED_PROJECT_DIR/build/text_editor" ned-1.0.0/usr/bin/ned.bin
echo "Copying fonts..."
cp -rv "$NED_PROJECT_DIR/fonts/"* ned-1.0.0/usr/share/ned/fonts/
echo "Copying icons..."
cp -rv "$NED_PROJECT_DIR/icons/"* ned-1.0.0/usr/share/ned/icons/

# Create wrapper script
echo "Creating wrapper script..."
cat > ned-1.0.0/usr/bin/ned << 'EOL'
#!/bin/bash
cd /usr/share/ned
exec /usr/bin/ned.bin "$@"
EOL

echo "Setting permissions..."
chmod -v +x ned-1.0.0/usr/bin/ned
chmod -v +x ned-1.0.0/usr/bin/ned.bin

# Build the package
echo "Building package..."
dpkg-deb --build ned-1.0.0

echo "Package built: $PACKAGE_DIR/ned-1.0.0.deb"
