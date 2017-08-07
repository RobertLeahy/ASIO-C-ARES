#!/bin/bash
set -x
mkdir cares
pushd cares
wget -nc https://github.com/c-ares/c-ares/archive/master.zip
unzip master.zip
pushd c-ares-master
mkdir build
pushd build
cmake .. -DCARES_STATIC=On -DCARES_SHARED=Off -DCMAKE_BUILD_TYPE=Release
cmake --build .
cp ./ares_*.h ../
popd
popd
popd
