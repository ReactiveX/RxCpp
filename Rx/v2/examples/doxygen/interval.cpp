#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("immediate interval sample"){
    printf("//! [immediate interval sample]\n");
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::interval(period);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [immediate interval sample]\n");
}

SCENARIO("threaded immediate interval sample"){
    printf("//! [threaded immediate interval sample]\n");
    auto scheduler = rxcpp::identity_current_thread();
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::interval(period, scheduler);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded immediate interval sample]\n");
}

SCENARIO("interval sample"){
    printf("//! [interval sample]\n");
    auto start = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::interval(start, period);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [interval sample]\n");
}

SCENARIO("threaded interval sample"){
    printf("//! [threaded interval sample]\n");
    auto scheduler = rxcpp::identity_current_thread();
    auto start = scheduler.now() + std::chrono::milliseconds(1);
    auto period = std::chrono::milliseconds(1);
    auto values = rxcpp::observable<>::interval(start, period, scheduler);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded interval sample]\n");
}
