#!/bin/bash
# Build and run ned unit tests.
# Suites: Monaco model ports, EditorOperations, EditorCommands, ProjectUndo,
#         SaveService, GitLibgit2, UTF-8 helpers.
# Usage:
#   ./scripts/test.sh                 # configure (if needed), build, run all
#   ./scripts/test.sh --clean         # wipe build dir first
#   ./scripts/test.sh --list          # list test cases without running
#   ./scripts/test.sh "[ned]"         # layer tests only (commands/undo/save/…)
#   ./scripts/test.sh "[monaco]"      # Monaco-shaped model ports only
#   ./scripts/test.sh "[ned][commands]"  # Catch2 tag filters

cd "$(dirname "$0")/.."

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

BUILD_DIR=".build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

CLEAN=0
LIST=0
CATCH_ARGS=()

for arg in "$@"; do
	case "$arg" in
	--clean)
		CLEAN=1
		;;
	--list)
		LIST=1
		;;
	-h | --help)
		echo "Usage: $0 [--clean] [--list] [catch2 filters...]"
		echo "  --clean   Remove ${BUILD_DIR} before configuring"
		echo "  --list    List tests (ned_tests --list-tests)"
		echo "  filters   Catch2 tags/names, e.g.:"
		echo "              [ned]  [ned][commands]  [ned][undo]  [ned][save]"
		echo "              [ned][git]  [ned][utf8]"
		echo "              [monaco]  [editOp]  [applyEdits]"
		exit 0
		;;
	*)
		CATCH_ARGS+=("$arg")
		;;
	esac
done

if [ "$CLEAN" -eq 1 ]; then
	echo -e "${BLUE}Cleaning ${BUILD_DIR}...${NC}"
	rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

if [[ "$OSTYPE" == "darwin"* ]]; then
	export MACOSX_DEPLOYMENT_TARGET=11.0
fi

# Reconfigure when tests target is missing or CMake cache is absent.
NEED_CMAKE=0
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
	NEED_CMAKE=1
elif ! grep -q 'ned_tests' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null &&
	[ ! -f "$BUILD_DIR/tests/CMakeFiles/ned_tests.dir/build.make" ]; then
	NEED_CMAKE=1
fi
# Always ensure tests are enabled.
if [ "$NEED_CMAKE" -eq 1 ] || ! grep -q 'NED_BUILD_TESTS:BOOL=ON' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
	echo -e "${BLUE}Configuring (NED_BUILD_TESTS=ON)...${NC}"
	cmake -S . -B "$BUILD_DIR" -DNED_BUILD_TESTS=ON
	if [ $? -ne 0 ]; then
		echo -e "${RED}CMake configure failed${NC}"
		exit 1
	fi
fi

echo -e "${BLUE}Building ned_tests (-j${JOBS})...${NC}"
cmake --build "$BUILD_DIR" --target ned_tests -j"$JOBS"
if [ $? -ne 0 ]; then
	echo -e "${RED}Build failed${NC}"
	exit 1
fi

TEST_BIN="$BUILD_DIR/tests/ned_tests"
if [ ! -x "$TEST_BIN" ]; then
	# Fallback path (some generators place the binary differently)
	TEST_BIN=$(find "$BUILD_DIR" -name ned_tests -type f -perm -111 2>/dev/null | head -1)
fi
if [ -z "$TEST_BIN" ] || [ ! -x "$TEST_BIN" ]; then
	echo -e "${RED}ned_tests binary not found under ${BUILD_DIR}${NC}"
	exit 1
fi

if [ "$LIST" -eq 1 ]; then
	echo -e "${BLUE}Listing tests...${NC}"
	"$TEST_BIN" --list-tests
	exit $?
fi

echo -e "${BLUE}Running ${TEST_BIN}${NC}"
if [ ${#CATCH_ARGS[@]} -gt 0 ]; then
	echo -e "${YELLOW}Filters: ${CATCH_ARGS[*]}${NC}"
	"$TEST_BIN" "${CATCH_ARGS[@]}"
else
	"$TEST_BIN"
fi
status=$?

if [ $status -eq 0 ]; then
	echo -e "${GREEN}All tests passed${NC}"
else
	echo -e "${RED}Tests failed (exit ${status})${NC}"
fi
exit $status
