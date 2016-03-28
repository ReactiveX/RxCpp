#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("exists sample"){
    printf("//! [exists sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3, 4, 5).exists([](int n) { return n > 3; });
    values.
            subscribe(
            [](bool v){ printf("OnNext: %s\n", v ? "true" : "false"); },
            [](){ printf("OnCompleted\n"); });
    printf("//! [exists sample]\n");
}