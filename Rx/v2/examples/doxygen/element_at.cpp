#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("element_at sample"){
    printf("//! [element_at sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).element_at(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [element_at sample]\n");
}
