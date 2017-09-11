The Reactive Extensions for C++ (__RxCpp__) is a library of algorithms for values-distributed-in-time. The [__Range-v3__](https://github.com/ericniebler/range-v3) library does the same for values-distributed-in-space.

Platform    | Status | 
----------- | :------------ |
Windows | [![Windows Status](http://img.shields.io/appveyor/ci/kirkshoop/RxCpp-446.svg?style=flat-square)](https://ci.appveyor.com/project/kirkshoop/rxcpp-446)
Linux & OSX | [![Linux & Osx Status](http://img.shields.io/travis/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://travis-ci.org/Reactive-Extensions/RxCpp)

Source        | Badges |
------------- | :--------------- |
Github | [![GitHub license](https://img.shields.io/github/license/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://github.com/Reactive-Extensions/RxCpp) <br/> [![GitHub release](https://img.shields.io/github/release/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://github.com/Reactive-Extensions/RxCpp/releases) <br/> [![GitHub commits](https://img.shields.io/github/commits-since/Reactive-Extensions/RxCpp/v4.0.0.svg?style=flat-square)](https://github.com/Reactive-Extensions/RxCpp)
Gitter.im | [![Join in on gitter.im](https://img.shields.io/gitter/room/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://gitter.im/Reactive-Extensions/RxCpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
Packages | [![NuGet version](http://img.shields.io/nuget/v/RxCpp.svg?style=flat-square)](http://www.nuget.org/packages/RxCpp/) [![vcpkg port](https://img.shields.io/badge/vcpkg-port-blue.svg?style=flat-square)](https://github.com/Microsoft/vcpkg/tree/master/ports/rxcpp)
Documentation | [![rxcpp doxygen documentation](https://img.shields.io/badge/rxcpp-latest-brightgreen.svg?style=flat-square)](http://reactive-extensions.github.io/RxCpp) <br/> [![reactivex intro](https://img.shields.io/badge/reactivex.io-intro-brightgreen.svg?style=flat-square)](http://reactivex.io/intro.html) [![rx marble diagrams](https://img.shields.io/badge/rxmarbles-diagrams-brightgreen.svg?style=flat-square)](http://rxmarbles.com/)

# Example
Add ```Rx/v2/src``` to the include paths

[![lines from bytes](https://img.shields.io/badge/blog%20post-lines%20from%20bytes-blue.svg?style=flat-square)](http://kirkshoop.github.io/async/rxcpp/c++/2015/07/07/rxcpp_-_parsing_bytes_to_lines_of_text.html)

```cpp
#include "rxcpp/rx.hpp"
namespace Rx {
using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;
}
using namespace Rx;

#include <regex>
#include <random>
using namespace std;
using namespace std::chrono;

int main()
{
    random_device rd;   // non-deterministic generator
    mt19937 gen(rd());
    uniform_int_distribution<> dist(4, 18);

    // for testing purposes, produce byte stream that from lines of text
    auto bytes = range(0, 10) |
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
                    [](vector<uint8_t> v, uint8_t b){
                        v.push_back(b);
                        return v;
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
    
    auto removespaces = [](string s){
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
        return s;
    };

    // create strings split on \r
    auto strings = bytes |
        concat_map([](vector<uint8_t> v){
            string s(v.begin(), v.end());
            regex delim(R"/(\r)/");
            cregex_token_iterator cursor(&s[0], &s[0] + s.size(), delim, {-1, 0});
            cregex_token_iterator end;
            vector<string> splits(cursor, end);
            return iterate(move(splits));
        }) |
        filter([](const string& s){
            return !s.empty();
        }) |
        publish() |
        ref_count();

    // filter to last string in each line
    auto closes = strings |
        filter(
            [](const string& s){
                return s.back() == '\r';
            }) |
        Rx::map([](const string&){return 0;});

    // group strings by line
    auto linewindows = strings |
        window_toggle(closes | start_with(0), [=](int){return closes;});

    // reduce the strings for a line into one string
    auto lines = linewindows |
        flat_map([&](observable<string> w) {
            return w | start_with<string>("") | sum() | Rx::map(removespaces);
        });

    // print result
    lines |
        subscribe<string>(println(cout));

    return 0;
}
```

# Reactive Extensions

>The ReactiveX Observable model allows you to treat streams of asynchronous events with the same sort of simple, composable operations that you use for collections of data items like arrays. It frees you from tangled webs of callbacks, and thereby makes your code more readable and less prone to bugs.

Credit [ReactiveX.io](http://reactivex.io/intro.html)

### Other language implementations

* Java: [RxJava](https://github.com/ReactiveX/RxJava)
* JavaScript: [RxJS](https://github.com/Reactive-Extensions/RxJS)
* C#: [Rx.NET](https://github.com/Reactive-Extensions/Rx.NET)
* [More..](http://reactivex.io/languages.html)

### Resources

* [Intro](http://reactivex.io/intro.html)
* [Tutorials](http://reactivex.io/tutorials.html)
* [Marble Diagrams](http://rxmarbles.com/)
* [twitter stream analysis app](https://github.com/kirkshoop/twitter)
  * [![baldwin pass to wilson](https://img.youtube.com/vi/QkvCzShHyVU/0.jpg)](https://www.youtube.com/watch?v=QkvCzShHyVU)
* _Algorithm Design For Values Distributed In Time_
  * [![C++ Russia 2016](https://img.youtube.com/vi/Re6DS5Ff0uE/0.jpg)](https://www.youtube.com/watch?v=Re6DS5Ff0uE)
  * [![CppCon 2016](https://img.youtube.com/vi/FcQURwM806o/0.jpg)](https://www.youtube.com/watch?v=FcQURwM806o)

# Cloning RxCpp

RxCpp uses a git submodule (in `ext/catch`) for the excellent [Catch](https://github.com/philsquared/Catch) library. The easiest way to ensure that the submodules are included in the clone is to add `--recursive` in the clone command.

```shell
git clone --recursive https://github.com/Reactive-Extensions/RxCpp.git
cd RxCpp
```

# Building RxCpp

* RxCpp is regularly tested on OSX and Windows.
* RxCpp is regularly built with Clang, Gcc and VC
* RxCpp depends on the latest compiler releases.

RxCpp uses CMake to create build files for several platforms and IDE's

### ide builds

#### XCode
```shell
mkdir projects/build
cd projects/build
cmake -G"Xcode" ../CMake -B.
```

#### Visual Studio 2013
```batch
mkdir projects\build
cd projects\build
cmake -G"Visual Studio 14" ..\CMake -B.
msbuild rxcpp.sln
```

### makefile builds

#### OSX
```shell
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

#### Linux --- Clang
```shell
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

#### Linux --- GCC
```shell
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

#### Windows
```batch
mkdir projects\build
cd projects\build
cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ..\CMake
nmake
```

The build only produces test and example binaries.

# Running tests

* You can use the CMake test runner `ctest`
* You can run the test binaries directly `rxcpp_test_*`
* Tests can be selected by name or tag
Example of by-tag

`rxcpp_test_subscription [perf]`

# Documentation

RxCpp uses Doxygen to generate project [documentation](http://reactive-extensions.github.io/RxCpp).

When Doxygen+Graphviz is installed, CMake creates a special build task named `doc`. It creates actual documentation and puts it to `projects/doxygen/html/` folder, which can be published to the `gh-pages` branch. Each merged pull request will build the docs and publish them.

[Developers Material](DeveloperManual.md)

# Contributing Code

Before submitting a feature or substantial code contribution please  discuss it with the team and ensure it follows the product roadmap. Note that all code submissions will be rigorously reviewed and tested by the Rx Team, and only those that meet an extremely high bar for both quality and design/roadmap appropriateness will be merged into the source.

You will be prompted to submit a Contributor License Agreement form after submitting your pull request. This needs to only be done once for any Microsoft OSS project. Fill in the [Contributor License Agreement](https://cla2.msopentech.com/) (CLA).

# Microsoft Open Source Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments. 
