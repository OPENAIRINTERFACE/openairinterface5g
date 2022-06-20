#!/bin/bash

echo "to build the LDPC decoder headers: go to the build directory, and type"
echo "make/ninja ldpc_generators"
echo
echo "assuming your build directory is ran_build/build, I trigger building for"
echo "you now. The generated headers will be in ran_build/build/ldpc/generator_*/"
echo

cd $OPENAIR_HOME/cmake_targets/ran_build/build
make ldpc_generators || ninja ldpc_generators
