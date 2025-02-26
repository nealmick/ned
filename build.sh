#!/bin/bash -i  # Force interactive shell

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

BUILD_DIR=".build"

# Handle clean flag
if [ "$1" = "--clean" ]; then
    echo -e "${BLUE}🗑️  Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Build header
echo -e "${GREEN}"
cat << "EOF"
  _   _ ______ _____  
 | \ | |  ____|  __ \ 
 |  \| | |__  | |  | |
 | . ` |  __| | |  | |
 | |\  | |____| |__| |
 |_| \_|______|_____/                    
EOF
echo -e "${NC}"
./format.sh
# Build steps
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${GREEN}📦 Running cmake...${NC}"
cmake ..

echo -e "${GREEN}📦 Running make...${NC}"
make

# Check build status
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    echo -e "${RED}"
    cat << "EOF"
  ______                     
 |  ____|                    
 | |__   _ __ _ __ ___  _ __ 
 |  __| | '__| '__/ _ \| '__|
 | |____| |  | | | (_) | |   
 |______|_|  |_|  \___/|_|   
                             
EOF
    echo -e "${NC}"
    exit 1
fi

# Success banner
echo -e "${GREEN}"
cat << "EOF"
  ____        _ _ _   
 |  _ \      (_) | |  
 | |_) |_   _ _| | |_ 
 |  _ <| | | | | | __|
 | |_) | |_| | | | |_ 
 |____/ \__,_|_|_|\__|
EOF
echo -e "${NC}"

echo -e "${GREEN}✅  Launching NED  🚀 ${NC}"
./ned

echo -e "${GREEN}✅ All Done!${NC}"