#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("iterate sample"){
    printf("//! [iterate sample]\n");
    std::array< int, 3 > a={{1, 2, 3}};
    auto values = rxcpp::observable<>::iterate(a);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [iterate sample]\n");
}

SCENARIO("threaded iterate sample"){
    printf("//! [threaded iterate sample]\n");
    std::array< int, 3 > a={{1, 2, 3}};
    auto values = rxcpp::observable<>::iterate(a, rxcpp::observe_on_event_loop());
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded iterate sample]\n");
}
