#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("sample period sample") {
    printf("//! [sample period sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(2)).
        take(7).
        sample_with_time(std::chrono::milliseconds(4));
    values.
        subscribe(
            [](long v) {
                printf("OnNext: %ld\n", v);
            },
            []() { printf("OnCompleted\n"); });
    printf("//! [sample period sample]\n");
}
