#!/bin/sh
# only test for y_ fies for now
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
for i in $(find ./samples/JSONTestSuite -name 'y_*.json')
do
    printf "./build-debug/prettify/a.out %s " '"'"$i"'"' 
    if valgrind --error-exitcode=1 -q -- ./build-debug/prettify/a.out "$i" 2>/dev/null 1>/dev/null 
    then
        printf "${GREEN}PASSED${NC}\n"
    else
        printf "${RED}FAILED${NC}\n"
    fi
done
