FROM ubuntu:bionic

RUN apt-get update && apt-get install -y wget && apt-get install -y gnupg

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# Add llvm apt repo
RUN echo deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic main >/etc/apt/sources.list.d/llvm.list
RUN echo deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic main >>/etc/apt/sources.list.d/llvm.list

# Now update with llvm
RUN apt-get update

# Clang.
RUN apt-get install -y \
    build-essential \
    ca-certificates \
    clang-12 \
    clang-tools-12 \
    clang-format-12 \
    lldb-12 \
    lld-12 \
    libc++-12-dev \
    libc++abi-12-dev

# CMake.
RUN cd /usr/local/src \ 
    && wget https://cmake.org/files/v3.10/cmake-3.10.3.tar.gz \
    && tar xvf cmake-3.10.3.tar.gz \ 
    && cd cmake-3.10.3 \
    && ./bootstrap \
    && make \
    && make install \
    && cd .. \
    && rm -rf cmake*
