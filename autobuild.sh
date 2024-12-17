#!/bin/bash

set -e

rm -rf `pwd`/build/*

if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

cd `pwd`/build &&
    cmake .. &&
    make

cd ..

if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig
