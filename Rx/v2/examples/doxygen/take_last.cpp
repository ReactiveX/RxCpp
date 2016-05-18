#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"


SCENARIO("take_last sample"){
    printf("//! [take_last sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).take_last(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [take_last sample]\n");
}
