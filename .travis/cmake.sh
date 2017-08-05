#!/bin/bash
set -x
mkdir cmake
pushd cmake
wget -nc https://cmake.org/files/v3.8/cmake-3.8.1-Linux-x86_64.sh
chmod 755 cmake-3.8.1-Linux-x86_64.sh
sed -i -r "s/^interactive=TRUE$/interactive=FALSE/g" cmake-3.8.1-Linux-x86_64.sh
./cmake-3.8.1-Linux-x86_64.sh
popd
