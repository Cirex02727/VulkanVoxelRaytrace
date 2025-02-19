#!/bin/bash

find ./res/shaders/ -name "*" -not -name "*.spv" -not -name "*.shinc" -type f -exec sh -c 'glslc {} -o {}.spv --target-spv=spv1.4' \;
find ./res/shaders/ -name "*.spv" -type f -exec sh -c 'spirv-opt -O {} -o {}' \;
