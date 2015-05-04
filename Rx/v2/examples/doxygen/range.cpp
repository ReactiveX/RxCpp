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

#include <iostream>
int get_pid() {
    std::stringstream s;
    int id;
    s << std::this_thread::get_id();
    s >> id;
    return id;
}

SCENARIO("threaded range sample"){
    printf("//! [threaded range sample]\n");
    printf("[thread %d] Start task\n", get_pid());
    auto values = rxcpp::observable<>::range(1, 3, rxcpp::observe_on_new_thread());
    auto s = values.
        map([](int prime) { return std::make_tuple(get_pid(), prime);});
    s.
        as_blocking().
        subscribe(
            rxcpp::util::apply_to(
                [](const int pid, int v) {
                    printf("[thread %d] OnNext: %d\n", pid, v);
                }),
            [](){printf("[thread %d] OnCompleted\n", get_pid());});
    printf("[thread %d] Finish task\n", get_pid());
    printf("//! [threaded range sample]\n");
}

SCENARIO("subscribe_on range sample"){
    printf("//! [subscribe_on range sample]\n");
    printf("[thread %d] Start task\n", get_pid());
    auto values = rxcpp::observable<>::range(1, 3);
    auto s = values.
        subscribe_on(rxcpp::observe_on_new_thread()).
        map([](int prime) { return std::make_tuple(get_pid(), prime);});
    s.
        as_blocking().
        subscribe(
            rxcpp::util::apply_to(
                [](const int pid, int v) {
                    printf("[thread %d] OnNext: %d\n", pid, v);
                }),
            [](){printf("[thread %d] OnCompleted\n", get_pid());});
    printf("[thread %d] Finish task\n", get_pid());
    printf("//! [subscribe_on range sample]\n");
}


SCENARIO("range concat sample"){
    printf("//! [range concat sample]\n");

    auto values = rxcpp::observable<>::range(1); // infinite (until overflow) stream of integers

    auto s1 = values.
        take(3).
        map([](int prime) { return std::make_tuple("1:", prime);});

    auto s2 = values.
        take(3).
        map([](int prime) { return std::make_tuple("2:", prime);});

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
        map([](int prime) { return std::make_tuple("1:", prime);});

    auto s2 = values.
        map([](int prime) { return std::make_tuple("2:", prime);});

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
        map([](int prime) { std::this_thread::yield(); return std::make_tuple("1:", prime);});

    auto s2 = values.
        subscribe_on(threads).
        take(3).
        map([](int prime) { std::this_thread::yield(); return std::make_tuple("2:", prime);});

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
        map([](int prime) { std::this_thread::yield(); return std::make_tuple("1:", prime);});

    auto s2 = values.
        subscribe_on(threads).
        map([](int prime) { std::this_thread::yield(); return std::make_tuple("2:", prime);});

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

