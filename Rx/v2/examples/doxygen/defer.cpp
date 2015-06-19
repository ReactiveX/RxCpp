#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("defer sample"){
    printf("//! [defer sample]\n");
    auto observable_factory = [](){return rxcpp::observable<>::range(1, 3);};
    auto values = rxcpp::observable<>::defer(observable_factory);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [defer sample]\n");
}

