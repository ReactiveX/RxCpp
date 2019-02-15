#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

#include <atomic>
#include <array>

SCENARIO("publish_synchronized sample"){
    printf("//! [publish_synchronized sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50)).
        take(5).
        publish_synchronized(rxcpp::observe_on_new_thread());

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Another subscription from the beginning
    values.subscribe(
        [](long v){printf("[2] OnNext: %ld\n", v);},
        [](){printf("[2] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(75)).subscribe([&](long){
        values.subscribe(
            [](long v){printf("[3] OnNext: %ld\n", v);},
            [](){printf("[3] OnCompleted\n");});
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("//! [publish_synchronized sample]\n");
}

SCENARIO("publish subject sample"){
    printf("//! [publish subject sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        publish();

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Another subscription from the beginning
    values.subscribe(
        [](long v){printf("[2] OnNext: %ld\n", v);},
        [](){printf("[2] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(75)).subscribe([&](long){
        values.subscribe(
            [](long v){printf("[3] OnNext: %ld\n", v);},
            [](){printf("[3] OnCompleted\n");});
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("//! [publish subject sample]\n");
}

SCENARIO("publish behavior sample"){
    printf("//! [publish behavior sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        publish(0L);

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Another subscription from the beginning
    values.subscribe(
        [](long v){printf("[2] OnNext: %ld\n", v);},
        [](){printf("[2] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(75)).subscribe([&](long){
        values.subscribe(
            [](long v){printf("[3] OnNext: %ld\n", v);},
            [](){printf("[3] OnCompleted\n");});
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("//! [publish behavior sample]\n");
}

SCENARIO("publish diamond bgthread sample"){
    printf("//! [publish diamond bgthread sample]\n");

    /*
     * Implements the following diamond graph chain with publish+connect on a background thread.
     *
     *            Values
     *          /      \
     *        *2        *100
     *          \      /
     *            Merge
     */
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        publish();

    // Left side multiplies by 2.
    auto left = values.map(
        [](long v){printf("[1] OnNext: %ld -> %ld\n", v, v*2); return v * 2;} );

    // Right side multiplies by 100.
    auto right = values.map(
        [](long v){printf("[2] OnNext: %ld -> %ld\n", v, v*100); return v * 100; });

    // Merge the left,right sides together.
    // The items are emitted interleaved ... [left1, right1, left2, right2, left3, right3, ...].
    auto merged = left.merge(right);

    std::atomic<bool> completed{false};

    // Add subscription to see results
    merged.subscribe(
        [](long v) { printf("[3] OnNext: %ld\n", v); },
        [&]() { printf("[3] OnCompleted:\n"); completed = true; });

    // Start emitting
    values.connect();

    // Block until subscription terminates.
    while (!completed) {}

    // Note: consider using ref_count(other) in real code, it's more composable.

    printf("//! [publish diamond bgthread sample]\n");
}

SCENARIO("publish diamond samethread sample"){
    printf("//! [publish diamond samethread sample]\n");

    /*
     * Implements the following diamond graph chain with publish+connect diamond without using threads.
     *
     *            Values
     *          /      \
     *        *2        *100
     *          \      /
     *            Merge
     */

    std::array<int, 5> a={{1, 2, 3, 4, 5}};
    auto values = rxcpp::observable<>::iterate(a).
        publish();

    // Left side multiplies by 2.
    auto left = values.map(
        [](long v){printf("[1] OnNext: %ld -> %ld\n", v, v*2); return v * 2;} );

    // Right side multiplies by 100.
    auto right = values.map(
        [](long v){printf("[2] OnNext: %ld -> %ld\n", v, v*100); return v * 100; });

    // Merge the left,right sides together.
    // The items are emitted interleaved ... [left1, right1, left2, right2, left3, right3, ...].
    auto merged = left.merge(right);

    // Add subscription to see results
    merged.subscribe(
        [](long v) { printf("[3] OnNext: %ld\n", v); },
        [&]() { printf("[3] OnCompleted:\n"); });

    // Start emitting
    // - because there are no other threads here, the connect call blocks until the source
    //   calls on_completed.
    values.connect();

    // Note: consider using ref_count(other) in real code, it's more composable.

    printf("//! [publish diamond samethread sample]\n");
}

// see also examples/doxygen/ref_count.cpp for more diamond examples
