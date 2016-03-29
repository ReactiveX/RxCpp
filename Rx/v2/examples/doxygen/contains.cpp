#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("contains sample") {
    printf("//! [contains sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3, 4, 5).contains(3);
    values.
            subscribe(
            [](bool v) { printf("OnNext: %s\n", v ? "true" : "false"); },
            []() { printf("OnCompleted\n"); });
    printf("//! [contains sample]\n");
}

SCENARIO("contains - operator syntax sample") {
    using namespace rxcpp;
    using namespace rxcpp::sources;
    using namespace rxcpp::operators;

    printf("//! [contains - operator syntax sample]\n");
    auto values = range(1, 10)
        | contains(2);
    values.
            subscribe(
            [](bool v) { printf("OnNext: %s\n", v ? "true" : "false"); },
            []() { printf("OnCompleted\n"); });
    printf("//! [contains - operator syntax sample]\n");
}