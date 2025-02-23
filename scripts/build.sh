#!/bin/bash

clean_build=false

while getopts "c" opt; do
  case $opt in
    c)
      clean_build=true
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

if $clean_build; then
  sudo rm -rf build/
fi

(mkdir build || true ) && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
sudo make install 
cd ..
