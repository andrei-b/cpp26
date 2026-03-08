#!/bin/bash
cd /home/andrei/cpp26/demos/invokable/cmake-build-debug
rm -f invokable CMakeFiles/invokable.dir/main.cpp.o
ninja invokable
if [ $? -eq 0 ]; then
    echo "Build successful!"
    ./invokable
    echo "Program exit code: $?"
else
    echo "Build failed!"
fi

