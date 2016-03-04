#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("ignore_elements sample"){
    printf("//! [ignore_elements sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3, 4, 5).ignore_elements();
    values.
            subscribe(
            [](int v) { printf("OnNext: %d\n", v); },
            []() { printf("OnCompleted\n"); });
    printf("//! [ignore_elements sample]\n");
}

