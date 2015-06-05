#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("filter sample"){
    printf("//! [filter sample]\n");
    auto values = rxcpp::observable<>::range(1, 6).
        filter([](int v){
            return v % 2;
        });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [filter sample]\n");
}
