#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("skip sample"){
    printf("//! [skip sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).skip(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [skip sample]\n");
}
