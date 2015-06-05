#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("distinct_until_changed sample"){
    printf("//! [distinct_until_changed sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 2, 3, 3, 3, 4, 5, 5).distinct_until_changed();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [distinct_until_changed sample]\n");
}

