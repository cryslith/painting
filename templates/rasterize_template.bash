#!/bin/bash

if [ $# -lt 3 ]
then
    echo "usage: $0 NAME WIDTH HEIGHT [EXTENSION=pbm]"
    exit 1
fi

name=$(basename "$1" .svg)
dir=$(dirname "$1")
if [ -z "$4" ]
then
    ext=pbm
else
    ext=$4
fi

outpath="${dir}/${name}_${2}_${3}.${ext}"

inkscape -z -e >(convert - "${outpath}") -w "$2" -h "$3" "$1"
