#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

#include <array>

SCENARIO("ref_count other diamond sample"){
    printf("//! [ref_count other diamond sample]\n");

    /*
     * Implements the following diamond graph chain with publish+ref_count without using threads.
     * This version is composable because it does not use connect explicitly.
     *
     *            Values
     *          /      \
     *        *2        *100
     *          \      /
     *            Merge
     *             |
     *            RefCount
     */

    std::array<double, 5> a={{1.0, 2.0, 3.0, 4.0, 5.0}};
    auto values = rxcpp::observable<>::iterate(a)
        // The root of the chain is only subscribed to once.
        .tap([](double v) { printf("[0] OnNext: %lf\n", v); })
        .publish();

    auto values_to_long = values.map([](double v) { return (long) v; });

    // Left side multiplies by 2.
    auto left = values_to_long.map(
        [](long v) -> long {printf("[1] OnNext: %ld -> %ld\n", v, v*2); return v * 2L;} );

    // Right side multiplies by 100.
    auto right = values_to_long.map(
        [](long v) -> long {printf("[2] OnNext: %ld -> %ld\n", v, v*100); return v * 100L; });

    // Merge the left,right sides together.
    // The items are emitted interleaved ... [left1, right1, left2, right2, left3, right3, ...].
    auto merged = left.merge(right);

    // When this value is subscribed to, it calls connect on values.
    auto connect_on_subscribe = merged.ref_count(values);

    // This immediately starts emitting all values and blocks until they are completed.
    connect_on_subscribe.subscribe(
        [](long v) { printf("[3] OnNext: %ld\n", v); },
        [&]() { printf("[3] OnCompleted:\n"); });

    printf("//! [ref_count other diamond sample]\n");
}

// see also examples/doxygen/publish.cpp for non-ref_count diamonds
