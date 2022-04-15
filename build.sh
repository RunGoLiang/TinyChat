#!/bin/sh

set -x

cd ./build
rm -rf *
cmake ..
make