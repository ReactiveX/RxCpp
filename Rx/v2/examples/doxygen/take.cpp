#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"


SCENARIO("take sample"){
    printf("//! [take sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).take(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [take sample]\n");
}
