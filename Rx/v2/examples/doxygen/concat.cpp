#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("concat sample"){
    printf("//! [concat sample]\n");
    auto o1 = rxcpp::observable<>::range(1, 3);
    auto o2 = rxcpp::observable<>::just(4);
    auto o3 = rxcpp::observable<>::from(5, 6);
    auto values = o1.concat(o2, o3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [concat sample]\n");
}

SCENARIO("implicit concat sample"){
    printf("//! [implicit concat sample]\n");
    auto o1 = rxcpp::observable<>::range(1, 3);
    auto o2 = rxcpp::observable<>::just(4);
    auto o3 = rxcpp::observable<>::from(5, 6);
    auto base = rxcpp::observable<>::from(o1.as_dynamic(), o2, o3);
    auto values = base.concat();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [implicit concat sample]\n");
}

SCENARIO("threaded concat sample"){
    printf("//! [threaded concat sample]\n");
    auto o1 = rxcpp::observable<>::range(1, 3);
    auto o2 = rxcpp::observable<>::just(4);
    auto o3 = rxcpp::observable<>::from(5, 6);
    auto values = o1.concat(rxcpp::observe_on_new_thread(), o2, o3);
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded concat sample]\n");
}

SCENARIO("threaded implicit concat sample"){
    printf("//! [threaded implicit concat sample]\n");
    auto o1 = rxcpp::observable<>::range(1, 3);
    auto o2 = rxcpp::observable<>::just(4);
    auto o3 = rxcpp::observable<>::from(5, 6);
    auto base = rxcpp::observable<>::from(o1.as_dynamic(), o2, o3);
    auto values = base.concat(rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded implicit concat sample]\n");
}
