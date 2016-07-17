#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("switch_if_empty sample"){
    printf("//! [switch_if_empty sample]\n");

    auto values = rxcpp::observable<>::empty<int>()
            .switch_if_empty(rxcpp::observable<>::range(1, 5));

    values.subscribe(
            [](int v) { printf("OnNext: %d\n", v); },
            []() { printf("OnCompleted\n"); } );

    printf("//! [switch_if_empty sample]\n");
}

SCENARIO("switch_if_empty - operator syntax sample") {
    using namespace rxcpp;
    using namespace rxcpp::sources;
    using namespace rxcpp::operators;

    printf("//! [switch_if_empty - operator syntax sample]\n");
    auto values = empty<int>()
        | switch_if_empty(range(1, 5));

    values.subscribe(
            [](int v) { printf("OnNext: %d\n", v); },
            []() { printf("OnCompleted\n"); } );

    printf("//! [switch_if_empty - operator syntax sample]\n");
}
