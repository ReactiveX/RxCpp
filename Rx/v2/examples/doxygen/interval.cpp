#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("interval sample"){
    printf("//! [interval sample]\n");
    auto start = std::chrono::steady_clock::now();
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::interval(start, period);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [interval sample]\n");
}

SCENARIO("threaded empty interval sample"){
    printf("//! [threaded empty interval sample]\n");
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now();
    auto values = rxcpp::observable<>::interval(start, scheduler);
    auto s = values.
        map([&](int prime) { 
            return std::make_tuple(std::chrono::duration_cast<std::chrono::milliseconds>(scheduler.now() - start).count(), prime);
        });
    s.
        take(3).
        subscribe(
            rxcpp::util::apply_to(
                [](__int64 t, int v){printf("OnNext: %d. %ld ms after the first event\n", v, t);}),
            [](){printf("OnCompleted\n");});
    printf("//! [threaded empty interval sample]\n");
}

SCENARIO("threaded interval sample"){
    printf("//! [threaded interval sample]\n");
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now();
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::interval(start, period, scheduler);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded interval sample]\n");
}

