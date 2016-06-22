#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("delay period+coordination sample"){
    printf("//! [delay period+coordination sample]\n");
    using namespace std::chrono;
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now();
    auto period = milliseconds(10);
    const auto next = [=](const char* s) {
        return [=](long v){
            auto t = duration_cast<milliseconds>(scheduler.now() - start);
            long long int ms = t.count();
            printf("[%s @ %lld] OnNext: %ld\n", s, ms, v);
        };
    };
    auto values = rxcpp::observable<>::interval(start, period, scheduler).
        take(4).
        tap(next("interval")).
        delay(period, rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            next(" delayed"),
            [](){printf("OnCompleted\n");});
    printf("//! [delay period+coordination sample]\n");
}

SCENARIO("delay period sample"){
    printf("//! [delay period sample]\n");
    using namespace std::chrono;
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now();
    auto period = milliseconds(10);
    const auto next = [=](const char* s) {
        return [=](long v){
            auto t = duration_cast<milliseconds>(scheduler.now() - start);
            long long int ms = t.count();
            printf("[%s @ %lld] OnNext: %ld\n", s, ms, v);
        };
    };
    auto values = rxcpp::observable<>::interval(start, period, scheduler).
        take(4).
        tap(next("interval")).
        delay(period);
    values.
        subscribe(
            next(" delayed"),
            [](){printf("OnCompleted\n");});
    printf("//! [delay period sample]\n");
}
