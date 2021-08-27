#!/bin/sh
set -ex

export CC=gcc-4.9
export CXX=g++-4.9

# Get Boost static libs
# Had to rebuild them using later GCC version. It breaks otherwise.
wget https://github.com/cpp3ds/3ds-tools/releases/download/Boost/boost-libs.tar.xz
tar xaf boost-libs.tar.xz

# Build nihstro
git clone https://github.com/neobrain/nihstro.git
cd nihstro && cmake -DBoost_USE_STATIC_LIBS=ON -DBOOST_LIBRARYDIR=$TRAVIS_BUILD_DIR/boost-libs . && make -j4 nihstro-assemble && make -j4 nihstro-disassemble && cd -

