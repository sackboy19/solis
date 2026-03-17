#!/bin/bash
set -e

clang++ -std=c++20 -g3 src/main.cpp -o solis

./solis