#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("skip_while sample"){
    printf("//! [skip_while sample]\n");
    auto values = rxcpp::observable<>::range(1, 8).
        skip_while([](int v){
                return v <= 4;
            });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [skip_while sample]\n");
}
