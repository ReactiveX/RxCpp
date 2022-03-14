#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("throttle sample"){
    printf("//! [throttle sample]\n");
    using namespace std::chrono;
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now();
    auto period = milliseconds(10);
    auto values = rxcpp::observable<>::interval(start, period, scheduler).
        take(4).
        throttle(period);
    values.
        subscribe(
            [](long v) { printf("OnNext: %ld\n", v); },
            []() { printf("OnCompleted\n"); });
    printf("//! [throttle sample]\n");
}