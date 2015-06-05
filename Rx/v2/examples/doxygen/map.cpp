#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("map sample"){
    printf("//! [map sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        map([](int v){
            return 2 * v;
        });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [map sample]\n");
}
