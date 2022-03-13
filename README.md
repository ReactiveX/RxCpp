The Reactive Extensions for C++ (__RxCpp__) is a library of algorithms for values-distributed-in-time. The [__Range-v3__](https://github.com/ericniebler/range-v3) library does the same for values-distributed-in-space.

Task    | Status | 
----------- | :------------ |
rxcpp CI | [![rxcpp CI](https://img.shields.io/github/workflow/status/ReactiveX/RxCpp/rxcpp%20CI/main.svg?event=push&style=flat-square)](https://github.com/ReactiveX/RxCpp/actions/workflows/rxcpp-ci.yml)

Source        | Badges |
------------- | :--------------- |
Github | [![GitHub license](https://img.shields.io/github/license/ReactiveX/RxCpp.svg?style=flat-square)](https://github.com/ReactiveX/RxCpp) <br/> [![GitHub release](https://img.shields.io/github/release/ReactiveX/RxCpp.svg?style=flat-square)](https://github.com/ReactiveX/RxCpp/releases) <br/> [![GitHub commits](https://img.shields.io/github/commits-since/ReactiveX/RxCpp/4.1.0.svg?style=flat-square)](https://github.com/ReactiveX/RxCpp)
Gitter.im | [![Join in on gitter.im](https://img.shields.io/gitter/room/Reactive-Extensions/RxCpp.svg?style=flat-square)](https://gitter.im/ReactiveX/RxCpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
Packages | [![NuGet version](http://img.shields.io/nuget/v/RxCpp.svg?style=flat-square)](http://www.nuget.org/packages/RxCpp/) [![vcpkg port](https://img.shields.io/badge/vcpkg-port-blue.svg?style=flat-square)](https://github.com/Microsoft/vcpkg/tree/master/ports/rxcpp)
Documentation | [![rxcpp doxygen documentation](https://img.shields.io/badge/rxcpp-latest-brightgreen.svg?style=flat-square)](http://reactivex.github.io/RxCpp) <br/> [![reactivex intro](https://img.shields.io/badge/reactivex.io-intro-brightgreen.svg?style=flat-square)](http://reactivex.io/intro.html) [![rx marble diagrams](https://img.shields.io/badge/rxmarbles-diagrams-brightgreen.svg?style=flat-square)](http://rxmarbles.com/)

# Usage

__RxCpp__ is a header-only C++ library that only depends on the standard library. The CMake build generates documentation and unit tests. The unit tests depend on a git submodule for the [Catch](https://github.com/philsquared/Catch) library.

# Example
Add `Rx/v2/src` to the include paths

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
        tap([](const vector<uint8_t>& v){
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
* JavaScript: [rxjs](https://github.com/ReactiveX/rxjs)
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
git clone --recursive https://github.com/ReactiveX/RxCpp.git
cd RxCpp
```

# Installing

To install RxCpp into your OS you need to follow standart procedure:
```shell
mkdir build
cd build
cmake ..
make install 
```

If you're using the vcpkg dependency manager, you can install RxCpp using a single one-line command:

```
vcpkg install rxcpp
```

Vcpkg will acquire RxCpp, build it from source in your computer, and provide CMake integration support for your projects.

See the [vcpkg repository](https://github.com/Microsoft/vcpkg) for more information.

# Importing

After you have successfully installed RxCpp you can import it into any project by simply adding to your CMakeLists.txt:
```cmake
find_package(rxcpp CONFIG)
``` 

# Building RxCpp Unit Tests

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

#### Visual Studio 2017
```batch
mkdir projects\build
cd projects\build
cmake -G "Visual Studio 15" ..\CMake\
msbuild Project.sln
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
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++" -B. ../CMake
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

RxCpp uses Doxygen to generate project [documentation](http://reactivex.github.io/RxCpp).

When Doxygen+Graphviz is installed, CMake creates a special build task named `doc`. It creates actual documentation and puts it to `projects/doxygen/html/` folder, which can be published to the `gh-pages` branch. Each merged pull request will build the docs and publish them.

[Developers Material](DeveloperManual.md)

# Contributing Code

Before submitting a feature or substantial code contribution please  discuss it with the team and ensure it follows the product roadmap. Note that all code submissions will be rigorously reviewed and tested by the Rx Team, and only those that meet an extremely high bar for both quality and design/roadmap appropriateness will be merged into the source.

# Microsoft Open Source Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments. 
