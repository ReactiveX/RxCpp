#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("scan sample"){
    printf("//! [scan sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).
        scan(
            0,
            [](int seed, int v){
                return seed + v;
            });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [scan sample]\n");
}
