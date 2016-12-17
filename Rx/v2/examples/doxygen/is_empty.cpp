#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("is_empty sample") {
    printf("//! [is_empty sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3, 4, 5).is_empty();
    values.
        subscribe(
        [](bool v) { printf("OnNext: %s\n", v ? "true" : "false"); },
        []() { printf("OnCompleted\n"); });
    printf("//! [is_empty sample]\n");
}
