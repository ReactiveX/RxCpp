#!/bin/sh

set -e

#if OS is linux or is not set
if [ "$TRAVIS_OS_NAME" = linux -o -z "$TRAVIS_OS_NAME" ]; then
    sudo apt-get clean
    sudo apt-get update
    sudo apt-get install -qq libc++
    sudo apt-get install cmake clang-3.5 libstdc++6
    sudo apt-get autoremove
elif [ "$TRAVIS_OS_NAME" = osx ]; then
    xcode-select --install
    brew update
    brew install cmake
fi
