#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("with_latest_from sample"){
    printf("//! [with_latest_from sample]\n");
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(2));
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(3));
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(5));
    auto values = o1.with_latest_from(o2, o3);
    values.
        take(5).
        subscribe(
            [](std::tuple<int, int, int> v){printf("OnNext: %d, %d, %d\n", std::get<0>(v), std::get<1>(v), std::get<2>(v));},
            [](){printf("OnCompleted\n");});
    printf("//! [with_latest_from sample]\n");
}

std::string get_pid();

SCENARIO("Coordination with_latest_from sample"){
    printf("//! [Coordination with_latest_from sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto thr = rxcpp::synchronize_event_loop();
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(2)).map([](int v) {
        printf("[thread %s] Source1 OnNext: %d\n", get_pid().c_str(), v);
        return v;
    });
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(3)).map([](int v) {
        printf("[thread %s] Source2 OnNext: %d\n", get_pid().c_str(), v);
        return v;
    });
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(5)).map([](int v) {
        printf("[thread %s] Source3 OnNext: %d\n", get_pid().c_str(), v);
        return v;
    });
    auto values = o1.with_latest_from(thr, o2, o3);
    values.
        take(5).
        as_blocking().
        subscribe(
            [](std::tuple<int, int, int> v){printf("[thread %s] OnNext: %d, %d, %d\n", get_pid().c_str(), std::get<0>(v), std::get<1>(v), std::get<2>(v));},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [Coordination with_latest_from sample]\n");
}

SCENARIO("Selector with_latest_from sample"){
    printf("//! [Selector with_latest_from sample]\n");
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(2));
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(3));
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(5));
    auto values = o1.with_latest_from(
        [](int v1, int v2, int v3) {
            return 100 * v1 + 10 * v2 + v3;
        },
        o2, o3);
    values.
        take(5).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [Selector with_latest_from sample]\n");
}

SCENARIO("Coordination+Selector with_latest_from sample"){
    printf("//! [Coordination+Selector with_latest_from sample]\n");
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(2));
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(3));
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(5));
    auto values = o1.with_latest_from(
        rxcpp::observe_on_new_thread(),
        [](int v1, int v2, int v3) {
            return 100 * v1 + 10 * v2 + v3;
        },
        o2, o3);
    values.
        take(5).
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [Coordination+Selector with_latest_from sample]\n");
}
