#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("distinct sample"){
    printf("//! [distinct sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 2, 3, 3, 3, 4, 5, 5).distinct();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [distinct sample]\n");
}

