#!/bin/bash

# out of source build
BUILD_DIR="build/"

# create and move to build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR
# build makefile
cmake ..
# build executable
make
# copy executable to base directory
cp oolong ../oolong

