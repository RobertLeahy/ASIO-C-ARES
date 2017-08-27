#!/bin/bash
set -x
mkdir cmake
pushd cmake
wget -nc https://cmake.org/files/v3.9/cmake-3.9.1-Linux-x86_64.sh
chmod 755 cmake-3.9.1-Linux-x86_64.sh
sed -i -r "s/^interactive=TRUE$/interactive=FALSE/g" cmake-3.9.1-Linux-x86_64.sh
./cmake-3.9.1-Linux-x86_64.sh
wget -nc https://raw.githubusercontent.com/Kitware/CMake/e710d6953dae27f73452095df6d7b2d7ec698fd1/Modules/FindBoost.cmake
cp FindBoost.cmake ./share/cmake-3.9/Modules/
popd
