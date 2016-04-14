#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("all sample") {
    printf("//! [all sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3, 4, 5).all([](int n) { return n < 6; });
    values.
            subscribe(
            [](bool v) { printf("OnNext: %s\n", v ? "true" : "false"); },
            []() { printf("OnCompleted\n"); });
    printf("//! [all sample]\n");
}

SCENARIO("all - operator syntax sample") {
    using namespace rxcpp;
    using namespace rxcpp::sources;
    using namespace rxcpp::operators;

    printf("//! [all - operator syntax sample]\n");
    auto values = range(1, 10)
        | all([](int n) { return n < 100; });
    values.
            subscribe(
            [](bool v) { printf("OnNext: %s\n", v ? "true" : "false"); },
            []() { printf("OnCompleted\n"); });
    printf("//! [all - operator syntax sample]\n");
}