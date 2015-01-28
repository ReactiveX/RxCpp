#!/bin/sh

set -e

#if OS is linux or is not set
if [ "$TRAVIS_OS_NAME" = linux -o -z "$TRAVIS_OS_NAME" ]; then
    wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
    sudo add-apt-repository -y 'deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty main'
    sudo add-apt-repository -y "deb http://us.archive.ubuntu.com/ubuntu/ trusty main universe"
    sudo add-apt-repository -y ppa:28msec/utils # Recent cmake
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test # gcc-4.8 backport for clang
    sudo apt-get clean -qq || echo "ignore clean failure"
    sudo apt-get update -qq || echo "ignore update failure"

    sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing cmake libssl-dev || echo "ignore install failure"

    if [ "$CC" = clang ]; then
        sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing clang-3.6 || echo "ignore install failure"

	sudo update-alternatives --remove-all clang   || echo "ignore remove failure"
	sudo update-alternatives --remove-all clang++ || echo "ignore remove failure"

	sudo update-alternatives --install /usr/bin/clang   clang   /usr/bin/clang-3.6 20
	sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.6 20

	sudo update-alternatives --config clang
	sudo update-alternatives --config clang++
    fi

    if [ "$CC" = gcc ]; then
        sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing gcc-4.8 g++-4.8 || echo "ignore install failure"

	sudo update-alternatives --remove-all gcc || echo "ignore remove failure"
	sudo update-alternatives --remove-all g++ || echo "ignore remove failure"

	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20

	sudo update-alternatives --config gcc
	sudo update-alternatives --config g++
    fi

elif [ "$TRAVIS_OS_NAME" = osx ]; then
    xcode-select --install
    brew update  || echo "suppress failures in order to ignore warnings"
    brew doctor || echo "suppress failures in order to ignore warnings"
    brew list cmake || brew install cmake || echo "suppress failures in order to ignore warnings"
fi
