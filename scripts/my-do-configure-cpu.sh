#!/bin/bash
# 
#
# Script for invoking CMake using the CMakeLists.txt file in this directory. 

# following lines for build directory cleanup
find . ! -name '*.sh' -type f -exec rm -f {} +
find . -mindepth 1 -type d -exec rm -rf {} +

cmake \
    -D CMAKE_CXX_COMPILER=mpic++ \
    -D CMAKE_INSTALL_PREFIX=./install \
    -D CompadreHarness_USE_Trilinos:BOOL=ON \
    -D CompadreHarness_USE_Trilinos_CXX_Flags:BOOL=ON \
    -D CompadreHarness_USE_Trilinos_Solvers:BOOL=ON \
    -D CompadreHarness_USE_Netcdf:BOOL=ON \
    -D CompadreHarness_USE_VTK:BOOL=ON \
    -D CompadreHarness_USE_PYTHON:BOOL=OFF \
    -D CompadreHarness_EXAMPLES:BOOL=ON \
    -D CompadreHarness_TESTS:BOOL=ON \
    -D CompadreHarness_CANGA_ENABLED:BOOL=ON \
    -D Toolkit_EXAMPLES:BOOL=ON \
    -D Toolkit_TESTS:BOOL=ON \
    -D Trilinos_PREFIX:FILEPATH="/ascldap/users/pakuber/releases/Trilinos/build2/install" \
    -D Netcdf_PREFIX:FILEPATH="/ascldap/users/pakuber/releases/netcdf" \
    -D VTK_PREFIX:FILEPATH="/ascldap/users/pakuber/releases/VTK" \
    \
    ..
