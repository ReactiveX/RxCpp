#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("default_if_empty sample"){
    printf("//! [default_if_empty sample]\n");

    auto values = rxcpp::observable<>::empty<int>()
            .default_if_empty(42);

    values.subscribe(
            [](int v) { printf("OnNext: %d\n", v); },
            []() { printf("OnCompleted\n"); } );

    printf("//! [default_if_empty sample]\n");
}
