#!/bin/bash
set -e

echo "Detecting latest compiler releases..."

GCC_VERSION=$(curl -s https://ftp.gnu.org/gnu/gcc/ | \
    grep -oP 'href="gcc-\d+\.\d+(?:\.\d+)?/' | \
    sed 's/href="gcc-//' | \
    sed 's/\///' | \
    sort -Vu | \
    tail -n 1)

if [ -z "$GCC_VERSION" ]; then
    echo "Failed to detect GCC version"
    exit 1
fi

LLVM_VERSION=$(curl -s https://api.github.com/repos/llvm/llvm-project/releases/latest | \
    python3 -c "import json, sys; data = json.load(sys.stdin); tag = data.get('tag_name', ''); print(tag.replace('llvmorg-', ''))")

if [ -z "$LLVM_VERSION" ]; then
    echo "Failed to detect LLVM version"
    exit 1
fi

echo "Detected versions:"
echo "GCC_VERSION=$GCC_VERSION"
echo "LLVM_VERSION=$LLVM_VERSION"

if [ -n "$GITHUB_OUTPUT" ]; then
    echo "GCC_VERSION=$GCC_VERSION" >> $GITHUB_OUTPUT
    echo "LLVM_VERSION=$LLVM_VERSION" >> $GITHUB_OUTPUT
fi
