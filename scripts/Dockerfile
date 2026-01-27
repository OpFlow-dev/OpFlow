ARG GCC_VERSION
ARG LLVM_VERSION

FROM ubuntu:24.04

ARG GCC_VERSION
ARG LLVM_VERSION

ENV DEBIAN_FRONTEND=noninteractive

# Use Tsinghua Tuna mirror for faster apt installs (Ubuntu 24.04 uses DEB822 sources).
RUN cp /etc/apt/sources.list.d/ubuntu.sources /etc/apt/sources.list.d/ubuntu.sources.bak && \
    cat > /etc/apt/sources.list.d/ubuntu.sources <<'EOF'
Types: deb
URIs: http://mirrors.tuna.tsinghua.edu.cn/ubuntu/
Suites: noble noble-updates noble-backports
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: http://mirrors.tuna.tsinghua.edu.cn/ubuntu/
Suites: noble-security
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF

RUN apt-get update && apt-get install -y \
    ca-certificates \
    wget \
    git \
    make \
    flex \
    bison \
    build-essential \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    texinfo \
    zlib1g-dev \
    libisl-dev \
    gnupg \
    lsb-release \
    software-properties-common \
    curl \
    ninja-build \
    cmake \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /gcc-build

RUN git clone --depth 1 --branch "releases/gcc-${GCC_VERSION}" --single-branch \
        https://mirrors.tuna.tsinghua.edu.cn/git/gcc.git gcc-source

WORKDIR /gcc-build/gcc-source

RUN mkdir -p /gcc-build/gcc-build
WORKDIR /gcc-build/gcc-build

RUN ../gcc-source/configure \
    --disable-multilib \
    --enable-languages=c,c++,fortran \
    --prefix=/usr/local \
    && make -j$(nproc) \
    && make install

WORKDIR /llvm-build
RUN git clone --depth 1 --branch "llvmorg-${LLVM_VERSION}" --single-branch \
        https://mirrors.tuna.tsinghua.edu.cn/git/llvm-project.git llvm-project

RUN mkdir -p /llvm-build/build
WORKDIR /llvm-build/build

RUN cmake -G Ninja \
    -DLLVM_ENABLE_PROJECTS="clang;lld;flang" \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_ENABLE_CXX_STD=ON \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    ../llvm-project/llvm && \
    ninja && \
    ninja install

ENV PATH="/usr/local/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64"
ENV CC=gcc
ENV CXX=g++
ENV FC=gfortran

RUN echo "=== GCC ===" && \
    gcc --version && \
    g++ --version && \
    gfortran --version && \
    echo "=== LLVM/Clang ===" && \
    clang --version && \
    clang++ --version && \
    flang --version && \
    echo "=== Versions ===" && \
    echo "GCC version: $(gcc --version | head -n1)" && \
    echo "Clang version: $(clang --version | head -n1)" && \
    echo "Flang version: $(flang --version | head -n1)"

WORKDIR /
RUN rm -rf /gcc-build /llvm-build

WORKDIR /workspace

LABEL maintainer="luohaothu"
LABEL description="Docker image with GCC ${GCC_VERSION} and LLVM/Clang ${LLVM_VERSION}"
LABEL gcc.version="${GCC_VERSION}"
LABEL llvm.version="${LLVM_VERSION}"
