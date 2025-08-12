# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

BUILD_DIR=".build"

# Handle clean flag
if [ "$1" = "--clean" ]; then
    echo "${BLUE}🗑️  Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Build header
echo "${GREEN}"
cat << "EOF"
  _   _ ______ _____  
 | \ | |  ____|  __ \ 
 |  \| | |__  | |  | |
 | . ` |  __| | |  | |
 | |\  | |____| |__| |
 |_| \_|______|_____/                       
EOF
echo "${NC}"

# Time the format script execution
echo "${BLUE}🎨 Running format script...${NC}"
start_time=$(date +%s.%N)
./format.sh
format_exit_code=$?
end_time=$(date +%s.%N)
format_duration=$(echo "$end_time - $start_time" | bc -l)
echo "${GREEN}✅ Format script completed in ${format_duration}s${NC}"

# Build steps
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "${GREEN}📦 Running cmake...${NC}"
# Set minimum macOS version for compatibility
if [[ "$OSTYPE" == "darwin"* ]]; then
    export MACOSX_DEPLOYMENT_TARGET=11.0
    echo "${BLUE}🍎 Setting macOS deployment target to 11.0${NC}"
fi
cmake ..

echo "${GREEN}📦 Running make...${NC}"
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Check build status
if [ $? -ne 0 ]; then
    echo "${RED}❌ Build failed!${NC}"
    echo "${RED}"
    cat << "EOF"
  ______                     
 |  ____|                    
 | |__   _ __ _ __ ___  _ __ 
 |  __| | '__| '__/ _ \| '__|
 | |____| |  | | | (_) | |   
 |______|_|  |_|  \___/|_|   
                             
EOF
    echo "${NC}"
    exit 1
fi

# Success banner
echo "${GREEN}"
cat << "EOF"
  ____        _ _ _   
 |  _ \      (_) | |  
 | |_) |_   _ _| | |_ 
 |  _ <| | | | | | __|
 | |_) | |_| | | | |_ 
 |____/ \__,_|_|_|\__|
EOF
echo "${NC}"

# Create the symbolic link for LSP support
echo "${BLUE}🔗 Setting up LSP support...${NC}"
cd ..
if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    # Remove existing symlink if it exists
    if [ -L "compile_commands.json" ]; then
        rm compile_commands.json
    fi
    # Create new symlink
    ln -s "$BUILD_DIR/compile_commands.json" .
    echo "${GREEN}✅ LSP configuration successful!${NC}"
else
    echo "${YELLOW}⚠️  No compile_commands.json found in build directory.${NC}"
fi

echo "${GREEN}✅  Launching NED  🚀 ${NC}"
./$BUILD_DIR/ned

echo "${GREEN}✅ Process terminated!${NC}"
echo ""
