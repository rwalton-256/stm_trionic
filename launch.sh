#!/bin/bash

set -e

cd build

cmake ..
make -j 20

cd ..

gdb-multiarch build/lesser_nighthawk -x gdb_commands
