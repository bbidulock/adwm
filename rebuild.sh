#!/bin/bash

rm -f cscope.*
./autogen.sh
./configure.sh
make clean all cscope
