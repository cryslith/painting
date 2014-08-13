#!/bin/bash

if [ $# -lt 3 ]
then
    echo "usage: $0 NAME WIDTH HEIGHT [EXTENSION=pbm]"
fi

name=$(basename "$1" .svg)
if [ -z "$4" ]
then
    ext=pbm
else
    ext=$4
fi

inkscape -z -e >(convert - "${name}_${2}_${3}.${ext}") -w "$2" -h "$3" "$1"
