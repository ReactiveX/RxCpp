#!/bin/sh

set -e

#if OS is linux or is not set
if [ "$TRAVIS_OS_NAME" = linux -o -z "$TRAVIS_OS_NAME" ]; then
    wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
    sudo add-apt-repository -y 'deb http://llvm.org/apt/precise/ llvm-toolchain-precise main'
    sudo add-apt-repository -y ppa:28msec/utils # Recent cmake
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test # gcc-4.8 backport for clang-3.5
    sudo apt-get clean -qq || echo "ignore clean failure"
    sudo apt-get update -qq || echo "ignore update failure"

    sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing cmake libssl-dev

    if [ "$CC" = clang ]; then
        sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing clang-3.6
        sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-3.6 20
        sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.6 20
    fi

    if [ "$CC" = gcc ]; then
        sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing gcc-4.8 g++-4.8
        sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
        sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20
    fi

elif [ "$TRAVIS_OS_NAME" = osx ]; then
    xcode-select --install
    brew update  || echo "suppress failures in order to ignore warnings"
    brew doctor || echo "suppress failures in order to ignore warnings"
    brew list cmake || brew install cmake || echo "suppress failures in order to ignore warnings"
fi
