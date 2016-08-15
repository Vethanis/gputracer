#!/bin/bash
find *h *.cpp *.glsl *.txt -type f -exec sed -i.orig 's/\t/    /g' {} +