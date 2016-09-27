#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("range sample"){
    printf("//! [range sample]\n");
    auto values1 = rxcpp::observable<>::range(1, 5);
    values1.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [range sample]\n");
}

std::string get_pid();

SCENARIO("threaded range sample"){
    printf("//! [threaded range sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto values = rxcpp::observable<>::range(1, 3, rxcpp::observe_on_new_thread());
    auto s = values.
        map([](int v) { return std::make_tuple(get_pid(), v);});
    s.
        as_blocking().
        subscribe(
            rxcpp::util::apply_to(
                [](const std::string pid, int v) {
                    printf("[thread %s] OnNext: %d\n", pid.c_str(), v);
                }),
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded range sample]\n");
}

SCENARIO("subscribe_on range sample"){
    printf("//! [subscribe_on range sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto values = rxcpp::observable<>::range(1, 3);
    auto s = values.
        subscribe_on(rxcpp::observe_on_new_thread()).
        map([](int v) { return std::make_tuple(get_pid(), v);});
    s.
        as_blocking().
        subscribe(
            rxcpp::util::apply_to(
                [](const std::string pid, int v) {
                    printf("[thread %s] OnNext: %d\n", pid.c_str(), v);
                }),
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [subscribe_on range sample]\n");
}


SCENARIO("range concat sample"){
    printf("//! [range concat sample]\n");

    auto values = rxcpp::observable<>::range(1); // infinite (until overflow) stream of integers

    auto s1 = values.
        take(3).
        map([](int v) { return std::make_tuple("1:", v);});

    auto s2 = values.
        take(3).
        map([](int v) { return std::make_tuple("2:", v);});

    s1.
        concat(s2).
        subscribe(rxcpp::util::apply_to(
            [](const char* s, int p) {
                printf("%s %d\n", s, p);
            }));
    printf("//! [range concat sample]\n");
}

SCENARIO("range merge sample"){
    printf("//! [range merge sample]\n");

    auto values = rxcpp::observable<>::range(1); // infinite (until overflow) stream of integers

    auto s1 = values.
        map([](int v) { return std::make_tuple("1:", v);});

    auto s2 = values.
        map([](int v) { return std::make_tuple("2:", v);});

    s1.
        merge(s2).
        take(6).
        as_blocking().
        subscribe(rxcpp::util::apply_to(
            [](const char* s, int p) {
                printf("%s %d\n", s, p);
            }));
    printf("//! [range merge sample]\n");
}

SCENARIO("threaded range concat sample"){
    printf("//! [threaded range concat sample]\n");
    auto threads = rxcpp::observe_on_event_loop();

    auto values = rxcpp::observable<>::range(1); // infinite (until overflow) stream of integers

    auto s1 = values.
        subscribe_on(threads).
        take(3).
        map([](int v) { std::this_thread::yield(); return std::make_tuple("1:", v);});

    auto s2 = values.
        subscribe_on(threads).
        take(3).
        map([](int v) { std::this_thread::yield(); return std::make_tuple("2:", v);});

    s1.
        concat(s2).
        observe_on(threads).
        as_blocking().
        subscribe(rxcpp::util::apply_to(
            [](const char* s, int p) {
                printf("%s %d\n", s, p);
            }));
    printf("//! [threaded range concat sample]\n");
}

SCENARIO("threaded range merge sample"){
    printf("//! [threaded range merge sample]\n");
    auto threads = rxcpp::observe_on_event_loop();

    auto values = rxcpp::observable<>::range(1); // infinite (until overflow) stream of integers

    auto s1 = values.
        subscribe_on(threads).
        map([](int v) { std::this_thread::yield(); return std::make_tuple("1:", v);});

    auto s2 = values.
        subscribe_on(threads).
        map([](int v) { std::this_thread::yield(); return std::make_tuple("2:", v);});

    s1.
        merge(s2).
        take(6).
        observe_on(threads).
        as_blocking().
        subscribe(rxcpp::util::apply_to(
            [](const char* s, int p) {
                printf("%s %d\n", s, p);
            }));
    printf("//! [threaded range merge sample]\n");
}

