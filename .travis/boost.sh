#!/bin/bash
set -x
mkdir boost
pushd boost
if [ ${2} = "64" ]; then BOOST_URL="https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz"
else BOOST_URL="https://downloads.sourceforge.net/project/boost/boost/1.${2}.0/boost_1_${2}_0.tar.gz"
fi
wget -nc ${BOOST_URL}
tar -zxf "boost_1_${2}_0.tar.gz"
pushd "boost_1_${2}_0"
./bootstrap.sh
if [ ${1} = "clang" ]; then PROPERTIES="cxxflags=-stdlib=libc++ linkflags=-stdlib=libc++"; fi
./b2 "--toolset=${1}" ${PROPERTIES} --with-system
popd
popd
