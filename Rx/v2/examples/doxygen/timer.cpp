#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("timepoint timer sample"){
    printf("//! [timepoint timer sample]\n");
    auto start = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::timer(start);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [timepoint timer sample]\n");
}

SCENARIO("duration timer sample"){
    printf("//! [duration timer sample]\n");
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::timer(period);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [duration timer sample]\n");
}

SCENARIO("threaded timepoint timer sample"){
    printf("//! [threaded timepoint timer sample]\n");
    auto scheduler = rxcpp::observe_on_new_thread();
    auto start = scheduler.now() + std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::timer(start, scheduler);
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded timepoint timer sample]\n");
}

SCENARIO("threaded duration timer sample"){
    printf("//! [threaded duration timer sample]\n");
    auto scheduler = rxcpp::observe_on_new_thread();
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::timer(period, scheduler);
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded duration timer sample]\n");
}
