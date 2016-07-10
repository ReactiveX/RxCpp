#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("empty sample"){
    printf("//! [empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [empty sample]\n");
}

SCENARIO("threaded empty sample"){
    printf("//! [threaded empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>(rxcpp::observe_on_event_loop());
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded empty sample]\n");
}

SCENARIO("empty operator syntax sample"){
    using namespace rxcpp::sources;

    printf("//! [empty operator syntax sample]\n");
    auto values = empty<int>();
    values.
            subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [empty operator syntax sample]\n");
}
