The Reactive Extensions for Native (__RxCpp__) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators in C++.

Windows: [![Windows Status](http://img.shields.io/appveyor/ci/kirkshoop/RxCpp-446.svg?style=flat-square)](https://ci.appveyor.com/project/kirkshoop/rxcpp-446)
Linux & OSX: [![Linux & Osx Status](http://img.shields.io/travis/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://travis-ci.org/Reactive-Extensions/RxCpp)

[![GitHub license](https://img.shields.io/github/license/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://github.com/Reactive-Extensions/RxCpp)

[![Join in on gitter.im](https://img.shields.io/gitter/room/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://gitter.im/Reactive-Extensions/RxCpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![doxygen documentation](https://img.shields.io/badge/documentation-latest-brightgreen.svg?style=flat-square)](http://reactive-extensions.github.io/RxCpp)

Github: [![GitHub release](https://img.shields.io/github/release/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://github.com/Reactive-Extensions/RxCpp/releases)
[![GitHub commits](https://img.shields.io/github/commits-since/Reactive-Extensions/RxCpp/v2.2.0.svg?style=flat-square)](https://github.com/Reactive-Extensions/RxCpp)

NuGet: [![NuGet version](http://img.shields.io/nuget/v/RxCpp.svg?style=flat-square)](http://www.nuget.org/packages/RxCpp/)
[![NuGet downloads](http://img.shields.io/nuget/dt/RxCpp.svg?style=flat-square)](http://www.nuget.org/packages/RxCpp/)

#Example
Add ```Rx/v2/src``` to the include paths

[![lines from bytes](https://img.shields.io/badge/blog%20post-lines%20from%20bytes-blue.svg?style=flat-square)](http://kirkshoop.github.io/async/rxcpp/c++/2015/07/07/rxcpp_-_parsing_bytes_to_lines_of_text.html)

```cpp
#include "rxcpp/rx.hpp"
using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;

#include <regex>
#include <random>
using namespace std;

int main()
{
    random_device rd;   // non-deterministic generator
    mt19937 gen(rd());
    uniform_int_distribution<> dist(4, 18);

    // for testing purposes, produce byte stream that from lines of text
    auto bytes = range(1, 10) |
        flat_map([&](int i){
            auto body = from((uint8_t)('A' + i)) |
                repeat(dist(gen)) |
                as_dynamic();
            auto delim = from((uint8_t)'\r');
            return from(body, delim) | concat();
        }) |
        window(17) |
        flat_map([](observable<uint8_t> w){
            return w |
                reduce(
                    vector<uint8_t>(),
                    [](vector<uint8_t>& v, uint8_t b){
                        v.push_back(b);
                        return move(v);
                    }) |
                as_dynamic();
        }) |
        tap([](vector<uint8_t>& v){
            // print input packet of bytes
            copy(v.begin(), v.end(), ostream_iterator<long>(cout, " "));
            cout << endl;
        });

    //
    // recover lines of text from byte stream
    //

    // create strings split on \r
    auto strings = bytes |
        concat_map([](vector<uint8_t> v){
            string s(v.begin(), v.end());
            regex delim(R"/(\r)/");
            sregex_token_iterator cursor(s.begin(), s.end(), delim, {-1, 0});
            sregex_token_iterator end;
            vector<string> splits(cursor, end);
            return iterate(move(splits));
        }) |
        filter([](string& s){
            return !s.empty();
        });

    // group strings by line
    int group = 0;
    auto linewindows = strings |
        group_by(
            [=](string& s) mutable {
                return s.back() == '\r' ? group++ : group;
            });

    // reduce the strings for a line into one string
    auto lines = linewindows |
        flat_map([](grouped_observable<int, string> w){
            return w | sum();
        });

    // print result
    lines |
        subscribe<string>(println(cout));

    return 0;
}
```

# Reactive Extensions:

* Rx.NET: The Reactive Extensions (Rx) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators.
* RxJS: The Reactive Extensions for JavaScript (RxJS) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators in JavaScript which can target both the browser and Node.js.
* RxCpp: The Reactive Extensions for Native (RxCpp) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators in both C++.

#Building RxCpp

* RxCpp is regularly tested on OSX and Windows.
* RxCpp is regularly built with Clang and VC
* RxCpp depends on the latest compiler releases.
* RxCpp has an experimental build with gcc.

RxCpp uses CMake to create build files for several platforms and IDE's

###Ide builds
####XCode
```shell
mkdir projects/build
cd projects/build
cmake -G"Xcode" ../CMake -B.
```

####Visual Studio 13
```batch
mkdir projects\build
cd projects\build
cmake -G"Visual Studio 12" ..\CMake -B.
```
* Note: open in VC2013 and upgrade to the 2013 toolset

###makefile builds

####OSX
```shell
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

####Linux --- Clang
```shell
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

####Linux --- GCC
```shell
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

####Windows
```batch
mkdir projects\build
cd projects\build
cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ..\CMake
nmake
```

The build only produces a test binary.

#Running tests

* You can use the CMake test runner ```ctest```
* You can run the test binary directly ```rxcppv2_test```
* Tests can be selected by name or tag
Example of by-tag

```rxcppv2_test [perf]```

#Documentation

RxCpp uses Doxygen to generate project [documentation](http://reactive-extensions.github.io/RxCpp).

When Doxygen+Graphviz is installed, CMake creates a special build task named ```doc```. It creates actual documentation and puts it to ```projects/doxygen/html/``` folder, which can be published to the `gh-pages` branch.

[Developers Material](DeveloperManual.md)

#Contributing Code

Before submitting a feature or substantial code contribution please  discuss it with the team and ensure it follows the product roadmap. Note that all code submissions will be rigorously reviewed and tested by the Rx Team, and only those that meet an extremely high bar for both quality and design/roadmap appropriateness will be merged into the source.

You will be promted to submit a Contributor License Agreement form after submitting your pull request. This needs to only be done once for any Microsoft OSS project. Fill in the [Contributor License Agreement](https://cla2.msopentech.com/) (CLA).
