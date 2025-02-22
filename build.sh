#!/bin/bash
# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Cleanup function
cleanup() {
   cd ..
   cat << "EOF"


   
 ------------------------------------------------------
  _______                  _             _           _ 
 |__   __|                (_)           | |         | |
    | | ___ _ __ _ __ ___  _ _ __   __ _| |_ ___  __| |
    | |/ _ \ '__| '_ ` _ \| | '_ \ / _` | __/ _ \/ _` |
    | |  __/ |  | | | | | | | | | | (_| | ||  __/ (_| |
    |_|\___|_|  |_| |_| |_|_|_| |_|\__,_|\__\___|\__,_|
 ------------------------------------------------------
                                                       
                                                                                 
EOF
echo -e "${NC}"
   exit
}

# Set up trap for Ctrl+C and other interrupts
trap cleanup INT TERM


# Print NED ASCII art in green
echo -e "${GREEN}"
cat << "EOF"
  _   _          _ 
 | \ | |        | |
 |  \| | ___  __| |
 | . ` |/ _ \/ _` |
 | |\  |  __/ (_| |
 |_| \_|\___|\__,_|
                   
EOF
echo -e "${NC}"

# Build steps with colored output
./format.sh

# Handle clean flag
if [ "$1" = "--clean" ]; then
   echo -e "${BLUE}🗑️  Cleaning build directory...${NC}"
   rm -rf build
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

echo -e "${YELLOW}📦 Building NED...${NC}"
cmake ..
make

# Check actual make exit status
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
   cd ..
   exit 1
fi
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


echo -e "${GREEN}🚀 Launching NED...${NC}"
./ned

# Return to original directory
cd ..
echo -e "${GREEN}✅ All Done!${NC}"