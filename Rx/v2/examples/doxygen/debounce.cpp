#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("debounce sample"){
    printf("//! [debounce sample]\n");
    using namespace std::chrono;
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now();
    auto period = milliseconds(10);
    auto values = rxcpp::observable<>::interval(start, period, scheduler).
        take(4).
        debounce(period);
    values.
        subscribe(
            [](long v) { printf("OnNext: %ld\n", v); },
            []() { printf("OnCompleted\n"); });
    printf("//! [debounce sample]\n");
}