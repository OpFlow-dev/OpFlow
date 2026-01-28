#!/bin/bash
set -euo pipefail

echo "Detecting latest compiler releases..."

curlGet() {
    curl -fsSL \
        --retry 5 \
        --retry-all-errors \
        --connect-timeout 10 \
        --max-time 30 \
        "$@"
}

detectGccVersionFromGit() {
    local repoUrl="$1"
    GIT_TERMINAL_PROMPT=0 git ls-remote --tags "$repoUrl" 'releases/gcc-*' 2>/dev/null | \
        awk '{print $2}' | \
        sed 's#^refs/tags/releases/gcc-##' | \
        sed 's/\^{}$//' | \
        grep -E '^[0-9]+\.[0-9]+(\.[0-9]+)?$' | \
        sort -Vu | \
        tail -n 1 || true
}

detectGccVersionFromFtpIndex() {
    local indexUrl="$1"
    curlGet "$indexUrl" | \
        grep -Eo 'href="gcc-[0-9]+\.[0-9]+(\.[0-9]+)?/' | \
        sed 's/href="gcc-//' | \
        sed 's#/$##' | \
        sort -Vu | \
        tail -n 1 || true
}

detectLlvmVersionFromGit() {
    local repoUrl="$1"
    GIT_TERMINAL_PROMPT=0 git ls-remote --tags "$repoUrl" 'llvmorg-*' 2>/dev/null | \
        awk '{print $2}' | \
        sed 's#^refs/tags/llvmorg-##' | \
        sed 's/\^{}$//' | \
        grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | \
        sort -Vu | \
        tail -n 1 || true
}

detectLlvmVersionFromGithubApi() {
    curlGet https://api.github.com/repos/llvm/llvm-project/releases/latest | \
        python3 -c "import json, sys; data = json.load(sys.stdin); tag = data.get('tag_name', ''); print(tag.replace('llvmorg-', ''))" || true
}

GCC_VERSION="$(detectGccVersionFromGit https://mirrors.tuna.tsinghua.edu.cn/git/gcc.git)"
if [ -z "${GCC_VERSION}" ]; then
    GCC_VERSION="$(detectGccVersionFromFtpIndex https://ftp.gnu.org/gnu/gcc/)"
fi
if [ -z "${GCC_VERSION}" ]; then
    GCC_VERSION="$(detectGccVersionFromFtpIndex https://ftpmirror.gnu.org/gcc/)"
fi

if [ -z "$GCC_VERSION" ]; then
    echo "Failed to detect GCC version"
    exit 1
fi

LLVM_VERSION="$(detectLlvmVersionFromGit https://mirrors.tuna.tsinghua.edu.cn/git/llvm-project.git)"
if [ -z "${LLVM_VERSION}" ]; then
    LLVM_VERSION="$(detectLlvmVersionFromGit https://github.com/llvm/llvm-project.git)"
fi
if [ -z "${LLVM_VERSION}" ]; then
    LLVM_VERSION="$(detectLlvmVersionFromGithubApi)"
fi

if [ -z "$LLVM_VERSION" ]; then
    echo "Failed to detect LLVM version"
    exit 1
fi

echo "Detected versions:"
echo "GCC_VERSION=$GCC_VERSION"
echo "LLVM_VERSION=$LLVM_VERSION"

githubOutputFile="${GITHUB_OUTPUT:-}"
if [ -n "$githubOutputFile" ]; then
    printf 'GCC_VERSION=%s\n' "$GCC_VERSION" >> "$githubOutputFile"
    printf 'LLVM_VERSION=%s\n' "$LLVM_VERSION" >> "$githubOutputFile"
fi
