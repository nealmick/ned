#!/bin/bash

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}📝 Starting code formatting...${NC}"

# Count and format files
count=$(find . \
   \( -name "*.cpp" -o -name "*.h" \) \
   -not -path "./lib/*" \
   -not -path "./fonts/*" \
   -not -path "./icons/*" \
   -not -path "./.build/*" \
   -not -path "./build/*" \
   -exec clang-format -i {} \; \
   -exec echo -n "." \; | wc -c)

echo -e "${GREEN}✨ Formatted ${count} files ${NC}"