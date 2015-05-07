#!/bin/sh

set -e

#if OS is linux or is not set
if [ "$TRAVIS_OS_NAME" = linux -o -z "$TRAVIS_OS_NAME" ]; then
    if [ "$CC" = gcc ]; then
        doxygen --version
        dot -V

        cd projects/build
        make doc -j2
#        cd ../doxygen/html
#        git init
#        git config user.name "KirkShoop"
#        git config user.email "kirkshoop@microsoft.com"
#        git add *
#        git commit -m "doxygen generated site"
#        git push --force "https://${GH_TOKEN}@github.com/Reactive-Extensions/RxCpp.git" master:gh-pages
    fi
fi
