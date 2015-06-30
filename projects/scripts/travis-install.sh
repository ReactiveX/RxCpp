#!/bin/sh

set -e

echo "TRAVIS_OS_NAME=$TRAVIS_OS_NAME"

#if OS is linux or is not set
if [ "$TRAVIS_OS_NAME" = linux -o -z "$TRAVIS_OS_NAME" ]; then

    wget http://www.cmake.org/files/v3.2/cmake-3.2.3-Linux-x86_64.sh
    chmod a+x cmake-3.2.3-Linux-x86_64.sh
    sudo ./cmake-3.2.3-Linux-x86_64.sh --skip-license --prefix=/usr/local
    export PATH=/usr/local/bin:$PATH

    cmake --version

elif [ "$TRAVIS_OS_NAME" = osx ]; then

    xcode-select --install
    brew update || echo "suppress failures in order to ignore warnings"
    brew doctor || echo "suppress failures in order to ignore warnings"
    brew list cmake || echo "suppress failures in order to ignore warnings"
    sudo brew uninstall --force cmake || "suppress failures in order to ignore warnings"
    brew search cmake || echo "suppress failures in order to ignore warnings"
    brew install cmake || echo "suppress failures in order to ignore warnings"
    brew upgrade cmake || echo "suppress failures in order to ignore warnings"

    cmake --version
fi

