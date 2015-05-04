#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("from sample"){
    printf("//! [from sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [from sample]\n");
}

SCENARIO("threaded from sample"){
    printf("//! [threaded from sample]\n");
    auto values = rxcpp::observable<>::from(rxcpp::observe_on_event_loop(), 1, 2, 3);
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded from sample]\n");
}

