#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("tap sample"){
    printf("//! [tap sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        tap(
			[](int v){printf("Tap -       OnNext: %d\n", v);},
            [](){printf("Tap -       OnCompleted\n");});
    values.
        subscribe(
            [](int v){printf("Subscribe - OnNext: %d\n", v);},
            [](){printf("Subscribe - OnCompleted\n");});
    printf("//! [tap sample]\n");
}

SCENARIO("error tap sample"){
    printf("//! [error tap sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        tap(
            [](int v){printf("Tap -       OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                printf("Tap -       OnError: %s\n", rxu::what(ep).c_str());
            },
            [](){printf("Tap -       OnCompleted\n");});
    values.
        subscribe(
            [](int v){printf("Subscribe - OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                printf("Subscribe - OnError: %s\n", rxu::what(ep).c_str());
            },
            [](){printf("Subscribe - OnCompleted\n");});
    printf("//! [error tap sample]\n");
}
