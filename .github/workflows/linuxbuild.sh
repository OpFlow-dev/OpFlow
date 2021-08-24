#!/bin/bash

sudo apt install cmake
mkdir -p build && cd build || exit 1
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_CXX_COMPILER=g++-11 \
      -DOPFLOW_WITH_HDF5=ON -DBENCHMARK_ENABLE_TESTING=OFF -DOPFLOW_BUILD_ALL=ON \
      -DOPFLOW_WITH_VTK=OFF ..
make -j AllTests AllExamples AllBenchmarks