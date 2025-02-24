#!/bin/bash

clean_build=false
enable_debug=false

BUILD_FLAGS=""

while getopts "cd" opt; do
  case $opt in
    c)
      clean_build=true
      ;;
    d)
      enable_debug=true
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

if $clean_build; then
  sudo rm -rf build/
fi

if $enable_debug; then
  echo "Enable Debug"
  BUILD_FLAGS="-DCMAKE_BUILD_TYPE=Debug"
fi

(mkdir build || true ) && cd build
cmake ${BUILD_FLAGS} ..
make
sudo make install 
cd ..
