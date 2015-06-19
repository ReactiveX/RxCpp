#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("just sample"){
    printf("//! [just sample]\n");
    auto values = rxcpp::observable<>::just(1);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [just sample]\n");
}

SCENARIO("threaded just sample"){
    printf("//! [threaded just sample]\n");
    auto values = rxcpp::observable<>::just(1, rxcpp::observe_on_event_loop());
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded just sample]\n");
}
