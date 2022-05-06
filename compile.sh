#!/bin/bash
cmake -G "Unix Makefiles" -S . -B ./build
cd ./build
make
./lab17_ap