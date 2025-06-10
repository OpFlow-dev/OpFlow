# 使用 Ubuntu 24.04 作为基础镜像
FROM ubuntu:24.04

# 设置非交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 安装编译 GCC 所需的依赖
RUN apt-get update && apt-get install -y \
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
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /gcc-build

# 克隆 GCC 源码
RUN git clone --depth 1 git://gcc.gnu.org/git/gcc.git gcc-source

# 创建编译目录
RUN mkdir gcc-build

# 设置编译参数并开始编译安装
WORKDIR /gcc-build/gcc-build
RUN ../gcc-source/configure \
    --disable-multilib \
    --enable-languages=c,c++,fortran \
    --prefix=/usr/local \
    && make -j$(nproc) \
    && make install

# 设置环境变量
ENV PATH="/usr/local/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/local/lib64:${LD_LIBRARY_PATH}"

# 验证安装
RUN gcc --version && g++ --version && gfortran --version

# 清理编译文件以减小镜像大小
WORKDIR /
RUN rm -rf /gcc-build

# 设置默认工作目录
WORKDIR /workspace