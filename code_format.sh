#!/bin/sh
find . \( -name '*.c' -o -name '*.h' \) -exec indent --no-tabs --linux-style --line-length 90 --indent-level 4 -ppi 4 -bli0 \{\} \;
