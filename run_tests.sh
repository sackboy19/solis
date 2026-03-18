#!/bin/bash
clang++ -std=c++20 -O1 -g3 src/tests.cpp -o solis_tests
if [ $? -eq 0 ]; then
	./solis_tests
	rm solis_tests
fi