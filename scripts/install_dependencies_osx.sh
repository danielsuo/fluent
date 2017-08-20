#! /usr/bin/env bash

set -euo pipefail

install_gtools() {
  brew install gflags glog protobuf
}
main() {
  set -x
  brew update
  brew install boost
  brew install zmq
  brew install cereal
  brew install redis
  brew install libev
  brew install hiredis
  brew install cmake
  install_gtools
  set +x
}

main
