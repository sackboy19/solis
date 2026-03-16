#!/bin/bash
set -e

clang++ -std=c++20 src/main.cpp -o solis

./solis