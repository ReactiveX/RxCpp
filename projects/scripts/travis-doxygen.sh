#!/bin/sh

set -e

if [ -n "$GH_TOKEN" ]; then
    cd projects/doxygen/html
    git init
    git config user.name "Kirk Shoop"
    git config user.email "kirk.shoop@microsoft.com"
    git add *
    git commit -m "doxygen generated site"
    git push --force "https://${GH_TOKEN}@github.com/Reactive-Extensions/RxCpp.git" master:gh-pages
fi
