#!/bin/sh
for i in $(find ./samples -name '*.json')
do
    valgrind --error-exitcode=1 -q -- ./build-debug/prettify/a.out "$i" >/dev/null \
        || echo "/build-debug/prettify/a.out" '"'"$i"'"' FAILED
done
