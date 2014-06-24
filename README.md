[![Build Status](https://travis-ci.org/Reactive-Extensions/RxCpp.png)](https://travis-ci.org/Reactive-Extensions/RxCpp)

# Reactive Extensions:

* Rx.NET: The Reactive Extensions (Rx) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators.
* RxJS: The Reactive Extensions for JavaScript (RxJS) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators in JavaScript which can target both the browser and Node.js.
* RxCpp: The Reactive Extensions for Native (RxC) is a library for composing asynchronous and event-based programs using observable sequences and LINQ-style query operators in both C and C++.

# Interactive Extensions
* Ix: The Interactive Extensions (Ix) is a .NET library which extends LINQ to Objects to provide many of the operators available in Rx but targeted for IEnumerable<T>.
* IxJS: An implementation of LINQ to Objects and the Interactive Extensions (Ix) in JavaScript.
* Ix++: An implantation of LINQ for Native Developers in C++

# Applications:
* Tx: a set of code samples showing how to use LINQ to events, such as real-time standing queries and queries on past history from trace and log files, which targets ETW, Windows Event Logs and SQL Server Extended Events.
* LINQ2Charts: an example for Rx bindings.  Similar to existing APIs like LINQ to XML, it allows developers to use LINQ to create/change/update charts in an easy way and avoid having to deal with XML or other underneath data structures. We would love to see more Rx bindings like this one.

#Building RxCpp

* RxCpp is regularly tested on OSX and Windows.
* RxCpp is regularly built with Clang and VC
* RxCpp depends on the latest compiler releases.
* RxCpp has an experimental build with gcc.

RxCpp uses CMake to create build files for several platforms and IDE's

###Ide builds
####XCode
```
mkdir projects/build
cd projects/build
cmake -G"Xcode" ../CMake -B.
```

####Visual Studio 13
```
mkdir projects\build
cd projects\build
cmake -G"Visual Studio 12" ..\CMake -B.
```
* Note: open in VC2013 and upgrade to the 2013 toolset

###makefile builds

####OSX
```
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

####Linux --- Clang
```
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

####Linux --- GCC
```
mkdir projects/build
cd projects/build
cmake -G"Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../CMake
make
```

####Windows
```
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

#Using RxCpp
Add ```Rx/v2/src``` to the include paths

```
#include "rxcpp/rx.hpp"
// create alias' to simplify code
// these are owned by the user so that
// conflicts can be managed by the user.
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;
namespace rxsub=rxcpp::subjects;

// At this time, RxCpp will fail to compile if the contents
// of the std namespace are merged into the global namespace
// DO NOT USE: 'using namespace std;'

#ifdef UNICODE
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    int c = 0;

    auto triples =
        rx::observable<>::range(1)
            .concat_map(
                [&c](int z){
                    return rx::observable<>::range(1, z)
                        .concat_map(
                            [=, &c](int x){
                                return rx::observable<>::range(x, z)
                                    .filter([=, &c](int y){++c; return x*x + y*y == z*z;})
                                    .map([=](int y){return std::make_tuple(x, y, z);})
                                    // forget type to workaround lambda deduction bug on msvc 2013
                                    .as_dynamic();},
                            [](int x, std::tuple<int,int,int> triplet){return triplet;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();},
                [](int z, std::tuple<int,int,int> triplet){return triplet;});

    int ct = 0;

    triples
        .take(100)
        .subscribe(rxu::apply_to([&ct](int x,int y,int z){
            ++ct;
        }));

    std::cout << "concat_map pythagorian range : " << c << " filtered to, " << ct << " triplets" << std::endl;

    return 0;
}
```

#Contributing Code

Before submitting a feature or substantial code contribution please  discuss it with the team and ensure it follows the product roadmap. Note that all code submissions will be rigorously reviewed and tested by the Rx Team, and only those that meet an extremely high bar for both quality and design/roadmap appropriateness will be merged into the source.

You will need to submit a  Contributor License Agreement form before submitting your pull request. This needs to only be done once for any Microsoft OSS project. Fill in the [Contributor License Agreement](https://cla.msopentech.com/) (CLA).
