#!/bin/bash

if [ ! -e paths.sh ]; then
    echo "You are not in 'tools' folder!"
    exit 1
fi
source paths.sh

set -x
clang-format -style=file -i ${INCLUDE_FILES} ${TEST_FILES}
