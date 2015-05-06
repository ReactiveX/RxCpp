#!/bin/sh

set -e

#if OS is linux or is not set
if [ "$TRAVIS_OS_NAME" = linux -o -z "$TRAVIS_OS_NAME" ]; then
#    wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
#    sudo add-apt-repository -y 'deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty main'
#    sudo add-apt-repository -y "deb http://us.archive.ubuntu.com/ubuntu/ trusty main universe"
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get clean -qq || echo "ignore clean failure"
    sudo apt-get update -qq || echo "ignore update failure"

    wget http://www.cmake.org/files/v3.1/cmake-3.1.3-Linux-x86_64.sh
    chmod a+x cmake-3.1.3-Linux-x86_64.sh
    sudo ./cmake-3.1.3-Linux-x86_64.sh --skip-license --prefix=/usr/local
    export PATH=/usr/local/bin:$PATH
    cmake --version

    if [ "$CC" = gcc ]; then
        sudo apt-get install -qq --allow-unauthenticated --force-yes --fix-missing gcc-4.8 g++-4.8 || echo "ignore install failure"

        sudo update-alternatives --remove-all gcc || echo "ignore remove failure"
        sudo update-alternatives --remove-all g++ || echo "ignore remove failure"

        sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
        sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20

        sudo update-alternatives --config gcc
        sudo update-alternatives --config g++

        g++ --version
        
        sudo apt-get install --allow-unauthenticated --force-yes --fix-missing doxygen
        sudo apt-get install --allow-unauthenticated --force-yes --fix-missing graphviz
        doxygen --version
        dot -V
    fi

elif [ "$TRAVIS_OS_NAME" = osx ]; then
    xcode-select --install
    brew update || echo "suppress failures in order to ignore warnings"
    brew doctor || echo "suppress failures in order to ignore warnings"
    brew list cmake || brew install cmake || echo "suppress failures in order to ignore warnings"
fi
