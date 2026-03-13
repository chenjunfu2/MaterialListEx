#!/bin/bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -S .
cmake --build build