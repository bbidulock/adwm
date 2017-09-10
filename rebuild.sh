#!/bin/bash

# a little script to rebuild the package in your working directory

rm -f cscope.*
./autogen.sh
./configure.sh
make clean
make cscope
cscope -b
make clean all README
