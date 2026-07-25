#!/bin/bash
# Build Ned.app + Ned.zip for distribution.
# Bundles Homebrew dylibs next to the binary using the *actual* sonames from otool
# (do not hardcode GLEW 2.2 — CI may have 2.3+).
set -euo pipefail
cd "$(dirname "$0")/.."

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [[ $(uname -m) == 'arm64' ]]; then
	HOMEBREW_PREFIX="${HOMEBREW_PREFIX:-/opt/homebrew}"
else
	HOMEBREW_PREFIX="${HOMEBREW_PREFIX:-/usr/local}"
fi

echo -e "${BLUE}Creating macOS app bundle...${NC}"

if [[ "$OSTYPE" == "darwin"* ]]; then
	export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
	echo -e "${BLUE}macOS deployment target: ${MACOSX_DEPLOYMENT_TARGET}${NC}"
fi

if [ ! -f ".build/ned" ]; then
	echo -e "${RED}Application not found. Run scripts/build.sh first.${NC}"
	exit 1
fi

APP_NAME="Ned"
APP_BUNDLE="$APP_NAME.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"
FRAMEWORKS="$CONTENTS/Frameworks"

rm -rf "$APP_BUNDLE"
mkdir -p "$MACOS" "$RESOURCES" "$FRAMEWORKS"

echo "Copying binary and resources..."
cp .build/ned "$MACOS/$APP_NAME"
cp -R resources "$RESOURCES/"
cp -R shaders "$RESOURCES/"
cp -R editor/services/highlight/queries "$RESOURCES/"
# Data files live under Resources (not MacOS/) so codesign is happy.
cp lib/imgui-terminal/rgb.txt "$RESOURCES/rgb.txt"
cp resources/icons/ned.icns "$RESOURCES/ned.icns"

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

BINARY="$MACOS/$APP_NAME"

# Resolve a dylib path on disk (Homebrew layouts vary).
find_dylib() {
	local name="$1"
	local candidates=(
		"$HOMEBREW_PREFIX/lib/$name"
		"$HOMEBREW_PREFIX/opt/glew/lib/$name"
		"$HOMEBREW_PREFIX/opt/glfw/lib/$name"
		"$HOMEBREW_PREFIX/opt/freetype/lib/$name"
		"$HOMEBREW_PREFIX/opt/libpng/lib/$name"
		"$HOMEBREW_PREFIX/opt/curl/lib/$name"
		"/usr/local/lib/$name"
	)
	local c
	for c in "${candidates[@]}"; do
		if [ -f "$c" ]; then
			echo "$c"
			return 0
		fi
	done
	# Cellar glob last
	local found
	found=$(ls "$HOMEBREW_PREFIX"/Cellar/*/*/lib/"$name" 2>/dev/null | head -1 || true)
	if [ -n "$found" ] && [ -f "$found" ]; then
		echo "$found"
		return 0
	fi
	return 1
}

# Copy one library into Frameworks (by absolute path or basename search).
bundle_dylib() {
	local src="$1"
	local base
	base=$(basename "$src")
	if [ ! -f "$src" ]; then
		if ! src=$(find_dylib "$base"); then
			echo -e "${YELLOW}Warning: could not find $base${NC}"
			return 1
		fi
	fi
	# Prefer the real file if this is a symlink (versioned soname).
	local real
	real=$(python3 -c "import os; print(os.path.realpath('$src'))" 2>/dev/null || echo "$src")
	cp -f "$real" "$FRAMEWORKS/$base"
	# If basename differs from real file name, still store under the soname the app expects.
	if [ "$(basename "$real")" != "$base" ]; then
		cp -f "$real" "$FRAMEWORKS/$base"
	fi
	chmod u+w "$FRAMEWORKS/$base" 2>/dev/null || true
	echo "Bundled: $base  (from $real)"
}

echo "Collecting linked Homebrew libraries from binary..."
# Absolute paths into homebrew / usr/local that the binary actually needs.
# (Avoid mapfile — macOS ships Bash 3.2.)
APP_LIBS=$(otool -L "$BINARY" | awk '/^\t/ {print $1}' | grep -E '/(opt/homebrew|usr/local)/' || true)

if [ -z "$APP_LIBS" ]; then
	echo -e "${YELLOW}No Homebrew/usr local dylibs found on binary (fully static?)${NC}"
fi

# Use for-loop so set -e / exit work (pipe+while runs in subshell on bash 3).
OLD_IFS=$IFS
IFS=$'\n'
for lib in $APP_LIBS; do
	IFS=$OLD_IFS
	[ -z "$lib" ] && continue
	base=$(basename "$lib")
	if [ ! -f "$FRAMEWORKS/$base" ]; then
		bundle_dylib "$lib" || true
	fi
	if [ -f "$FRAMEWORKS/$base" ]; then
		install_name_tool -change "$lib" "@executable_path/../Frameworks/$base" "$BINARY"
		echo "Rewrote app link: $base"
	else
		echo -e "${RED}Missing required library: $base (linked as $lib)${NC}"
		exit 1
	fi
done
IFS=$OLD_IFS

# Pull transitive deps (freetype → libpng, etc.) and fix @rpath / absolute refs.
echo "Bundling transitive dependencies..."
changed=1
pass=0
while [ "$changed" -eq 1 ] && [ "$pass" -lt 8 ]; do
	changed=0
	pass=$((pass + 1))
	for lib in "$FRAMEWORKS"/*.dylib; do
		[ -e "$lib" ] || continue
		while IFS= read -r dep; do
			[ -z "$dep" ] && continue
			dep_base=$(basename "$dep")
			if [ ! -f "$FRAMEWORKS/$dep_base" ]; then
				if bundle_dylib "$dep"; then
					changed=1
				fi
			fi
			if [ -f "$FRAMEWORKS/$dep_base" ]; then
				install_name_tool -change "$dep" "@loader_path/$dep_base" "$lib" 2>/dev/null || true
			fi
		done < <(otool -L "$lib" | awk '/^\t/ {print $1}' | grep -E '/(opt/homebrew|usr/local)/' || true)
		# Normalize install name of the dylib itself
		install_name_tool -id "@loader_path/$(basename "$lib")" "$lib" 2>/dev/null || true
	done
done

# Fail hard if the main binary still points outside the bundle.
LEFTOVER=$(otool -L "$BINARY" | awk '/^\t/ {print $1}' | grep -E '/(opt/homebrew|usr/local)/' || true)
if [ -n "$LEFTOVER" ]; then
	echo -e "${RED}Binary still has unbundled absolute library paths:${NC}"
	echo "$LEFTOVER"
	exit 1
fi

# Verify every @executable_path Frameworks reference exists.
while IFS= read -r ref; do
	name=$(basename "$ref")
	if [ ! -f "$FRAMEWORKS/$name" ]; then
		echo -e "${RED}Binary expects Frameworks/$name but it is missing${NC}"
		exit 1
	fi
done < <(otool -L "$BINARY" | awk '/@executable_path\/\.\.\/Frameworks\// {print $1}')

echo "Frameworks contents:"
ls -la "$FRAMEWORKS"

echo "Signing..."
for lib in "$FRAMEWORKS"/*.dylib; do
	[ -e "$lib" ] || continue
	codesign --force --sign - "$lib" 2>/dev/null || true
done
# Ad-hoc sign binary then bundle (avoid --deep; rgb.txt and resources are not code).
codesign --force --sign - "$BINARY"
codesign --force --sign - "$APP_BUNDLE"

echo "Architectures: $(file "$BINARY" | grep -o 'x86_64\|arm64' | tr '\n' ' ')"

ZIP_NAME="$APP_NAME.zip"
rm -f "$ZIP_NAME"
zip -r "$ZIP_NAME" "$APP_BUNDLE"

echo -e "${GREEN}Package created: $ZIP_NAME${NC}"
echo -e "${YELLOW}Tip: if Gatekeeper blocks the download: xattr -dr com.apple.quarantine Ned.app${NC}"
