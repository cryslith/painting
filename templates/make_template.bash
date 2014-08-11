#!/bin/bash

name=$(basename "$1" .svg)
inkscape -z -e >(convert - "${name}_${2}_${3}.pbm") -w "$2" -h "$3" "$1"
