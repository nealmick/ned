#!/bin/bash
# Build ned (Unix). Locally formats, configures, builds, and optionally runs the app.
# CI: set CI=true (GitHub Actions does) or pass --no-run to skip launching the GUI.
cd "$(dirname "$0")/.."

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

BUILD_DIR=".build"
NO_RUN=0
CLEAN=0
SKIP_FORMAT=0

# Optional features (default ON — same as CMake). Use --no-* / --minimal to trim.
ENABLE_TERMINAL=1
ENABLE_GIT=1
ENABLE_LSP=1
ENABLE_SHADERS=1

for arg in "$@"; do
	case "$arg" in
	--clean) CLEAN=1 ;;
	--no-run) NO_RUN=1 ;;
	--skip-format) SKIP_FORMAT=1 ;;
	--no-terminal) ENABLE_TERMINAL=0 ;;
	--no-git) ENABLE_GIT=0 ;;
	--no-lsp) ENABLE_LSP=0 ;;
	--no-shaders) ENABLE_SHADERS=0 ;;
	--minimal)
		# Editor core only: no terminal, git, LSP, or CRT shaders.
		ENABLE_TERMINAL=0
		ENABLE_GIT=0
		ENABLE_LSP=0
		ENABLE_SHADERS=0
		;;
	-h | --help)
		echo "Usage: $0 [options]"
		echo "  --clean         Wipe ${BUILD_DIR} first"
		echo "  --no-run        Build only (do not launch ned)"
		echo "  --skip-format   Skip clang-format"
		echo "  --no-terminal   Disable embedded terminal (imgui-terminal)"
		echo "  --no-git        Disable libgit2 gutter / status"
		echo "  --no-lsp        Disable language server client"
		echo "  --no-shaders    Disable CRT/burn-in postprocess"
		echo "  --minimal       All of --no-terminal --no-git --no-lsp --no-shaders"
		echo "  CI=true         Implies --no-run (and skips format if clang-format missing)"
		echo ""
		echo "Examples:"
		echo "  $0 --minimal --no-run"
		echo "  $0 --no-git --no-lsp"
		echo "  $0 --clean --minimal   # reconfigure from a full build"
		exit 0
		;;
	*)
		echo -e "${RED}Unknown option: $arg${NC}"
		echo "Try $0 --help"
		exit 1
		;;
	esac
done

# GitHub Actions / most CI systems set CI=true
if [ "${CI:-}" = "true" ] || [ "${CI:-}" = "1" ]; then
	NO_RUN=1
fi

if [ "$CLEAN" -eq 1 ]; then
	echo -e "${BLUE}Cleaning build directory...${NC}"
	rm -rf "$BUILD_DIR"
fi

# Time the format script execution (optional on CI)
if [ "$SKIP_FORMAT" -eq 0 ]; then
	if command -v clang-format >/dev/null 2>&1; then
		echo -e "${BLUE}Running format script...${NC}"
		./scripts/format.sh || true
	else
		echo -e "${YELLOW}clang-format not found; skipping format${NC}"
	fi
fi

# Build steps
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${GREEN}Running cmake...${NC}"
# Set minimum macOS version for compatibility
if [[ "$OSTYPE" == "darwin"* ]]; then
	export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
	echo -e "${BLUE}macOS deployment target: ${MACOSX_DEPLOYMENT_TARGET}${NC}"
fi

# Always pass feature flags so re-running with different --no-* updates the cache.
CMAKE_ARGS=(
	-DNED_ENABLE_TERMINAL="$ENABLE_TERMINAL"
	-DNED_ENABLE_GIT="$ENABLE_GIT"
	-DNED_ENABLE_LSP="$ENABLE_LSP"
	-DNED_ENABLE_SHADERS="$ENABLE_SHADERS"
)
if [ -n "${CMAKE_BUILD_TYPE:-}" ]; then
	CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE")
fi

echo -e "${BLUE}Features: terminal=${ENABLE_TERMINAL} git=${ENABLE_GIT} lsp=${ENABLE_LSP} shaders=${ENABLE_SHADERS}${NC}"

cmake .. "${CMAKE_ARGS[@]}"

if [ $? -ne 0 ]; then
	echo -e "${RED}CMake configure failed${NC}"
	exit 1
fi

echo -e "${GREEN}Building...${NC}"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build . -j"$JOBS"
if [ $? -ne 0 ]; then
	echo -e "${RED}Build failed${NC}"
	exit 1
fi

echo -e "${GREEN}Build succeeded${NC}"

# compile_commands for LSP
cd ..
if [ -f "$BUILD_DIR/compile_commands.json" ]; then
	if [ -L "compile_commands.json" ] || [ -f "compile_commands.json" ]; then
		rm -f compile_commands.json
	fi
	ln -s "$BUILD_DIR/compile_commands.json" .
	echo -e "${GREEN}Linked compile_commands.json${NC}"
fi

if [ ! -f "$BUILD_DIR/ned" ] && [ ! -f "$BUILD_DIR/Release/ned" ]; then
	# Some generators nest the binary
	if ! find "$BUILD_DIR" -name ned -type f | head -1 | grep -q .; then
		echo -e "${RED}ned binary not found under ${BUILD_DIR}${NC}"
		exit 1
	fi
fi

if [ "$NO_RUN" -eq 1 ]; then
	echo -e "${BLUE}Skipping launch (--no-run / CI)${NC}"
	exit 0
fi

echo -e "${GREEN}Launching NED...${NC}"
if [ -x "$BUILD_DIR/ned" ]; then
	./"$BUILD_DIR/ned"
elif [ -x "$BUILD_DIR/Release/ned" ]; then
	./"$BUILD_DIR/Release/ned"
else
	NED_BIN=$(find "$BUILD_DIR" -name ned -type f -perm -111 2>/dev/null | head -1)
	if [ -n "$NED_BIN" ]; then
		"$NED_BIN"
	else
		echo -e "${RED}Could not find ned to launch${NC}"
		exit 1
	fi
fi
