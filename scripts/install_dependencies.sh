#! /usr/bin/env bash

set -euo pipefail

install_misc() {
  # These dependencies are needed by a couple of different projects including
  # redis, protobuf, grpc, etc.
  sudo apt-get install \
    ant \
    autoconf \
    automake \
    curl \
    g++ \
    libcurl4-openssl-dev \
    libev-dev\
    libhiredis-dev \
    libssl-dev \
    libtool \
    libuv-dev \
    make \
    unzip \
    zlib1g-dev \
    cmake

}

install_clang() {
  sudo bash -c "echo '' >> /etc/apt/sources.list"
  sudo bash -c "echo '# http://apt.llvm.org/' >> /etc/apt/sources.list"
  sudo bash -c "echo 'deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-4.0 main' >> /etc/apt/sources.list"
  sudo bash -c "echo 'deb-src http://apt.llvm.org/trusty/ llvm-toolchain-trusty-4.0 main' >> /etc/apt/sources.list"
  wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
  sudo apt-get -y update

  sudo apt-get install -y \
    clang-4.0 clang-format-4.0 clang-tidy-4.0 lldb-4.0
  sudo ln -s "$(which clang-4.0)" /usr/bin/clang
  sudo ln -s "$(which clang++-4.0)" /usr/bin/clang++
  sudo ln -s "$(which clang-format-4.0)" /usr/bin/clang-format
  sudo ln -s "$(which clang-tidy-4.0)" /usr/bin/clang-tidy
  sudo ln -s "$(which lldb-4.0)" /usr/bin/lldb
  sudo ln -s "$(which lldb-server-4.0)" /usr/bin/lldb-server
}

install_gpp() {
  sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
  sudo apt-get update
  sudo apt-get install -y g++-6
  sudo ln -sf "$(which /usr/bin/g++-6)" /usr/bin/g++
  sudo ln -sf "$(which /usr/bin/gcc-6)" /usr/bin/gcc
}

install_boost() {
  sudo apt-get install libboost-dev
}

install_postgres() {
  sudo bash -c 'echo "deb http://apt.postgresql.org/pub/repos/apt/ trusty-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
  wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
  sudo apt-get update
  sudo apt-get install -y postgresql-9.6 postgresql-server-dev-9.6 python-dev
}

main() {
  set -x
  sudo apt-get -y update
  install_misc
  install_clang
  install_gpp
  install_boost
  install_postgres
  set +x
}

main
