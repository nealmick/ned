#!/usr/bin/env bash

# Format all .cpp and .h files, excluding lib/, fonts/, icons/, and build/ directories.
find . \
    \( -name "*.cpp" -o -name "*.h" \) \
    -not -path "./lib/*" \
    -not -path "./fonts/*" \
    -not -path "./icons/*" \
    -not -path "./build/*" \
    -exec echo "Formatting {}" \; \
    -exec clang-format -i {} \;

echo "Formatting complete."

