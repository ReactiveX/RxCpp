#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("zip sample"){
    printf("//! [zip sample]\n");
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(1));
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(2));
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(3));
    auto values = o1.zip(o2, o3);
    values.
        take(3).
        subscribe(
            [](std::tuple<int, int, int> v){printf("OnNext: %d, %d, %d\n", std::get<0>(v), std::get<1>(v), std::get<2>(v));},
            [](){printf("OnCompleted\n");});
    printf("//! [zip sample]\n");
}

std::string get_pid();

SCENARIO("Coordination zip sample"){
    printf("//! [Coordination zip sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto thr = rxcpp::synchronize_event_loop();
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(1)).map([](int v) {
        printf("[thread %s] Source1 OnNext: %d\n", get_pid().c_str(), v);
        return v;
    });
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(2)).map([](int v) {
        printf("[thread %s] Source2 OnNext: %d\n", get_pid().c_str(), v);
        return v;
    });
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(3)).map([](int v) {
        printf("[thread %s] Source3 OnNext: %d\n", get_pid().c_str(), v);
        return v;
    });
    auto values = o1.zip(thr, o2, o3);
    values.
        take(3).
        as_blocking().
        subscribe(
            [](std::tuple<int, int, int> v){printf("[thread %s] OnNext: %d, %d, %d\n", get_pid().c_str(), std::get<0>(v), std::get<1>(v), std::get<2>(v));},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [Coordination zip sample]\n");
}

SCENARIO("Selector zip sample"){
    printf("//! [Selector zip sample]\n");
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(1));
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(2));
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(3));
    auto values = o1 | rxcpp::operators::zip(
        [](int v1, int v2, int v3) {
            return 100 * v1 + 10 * v2 + v3;
        },
        o2, o3);
    values.
        take(3).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [Selector zip sample]\n");
}

SCENARIO("Coordination+Selector zip sample"){
    printf("//! [Coordination+Selector zip sample]\n");
    auto o1 = rxcpp::observable<>::interval(std::chrono::milliseconds(1));
    auto o2 = rxcpp::observable<>::interval(std::chrono::milliseconds(2));
    auto o3 = rxcpp::observable<>::interval(std::chrono::milliseconds(3));
    auto values = o1.zip(
        rxcpp::observe_on_new_thread(),
        [](int v1, int v2, int v3) {
            return 100 * v1 + 10 * v2 + v3;
        },
        o2, o3);
    values.
        take(3).
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [Coordination+Selector zip sample]\n");
}
