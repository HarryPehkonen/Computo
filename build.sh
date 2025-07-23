#!/bin/bash
rm -rf build
conan install . --output-folder=build --build=missing
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake
cmake --build build -j$(nproc)
