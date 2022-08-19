#!/bin/bash

if [ ! -e ../build ] || [ ! -e ../build/compile_commands.json ]; then
    printf "build directory or compile commands doesn't exists, linter needs cmake compile commands to continue:\
        \n mkdir ../build && cd ../build && cmake ..\n"
    exit 1
fi
source paths.sh

set -x
clang-tidy --format-style=file -p ../build/ ${INCLUDE_FILES}
