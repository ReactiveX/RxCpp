#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("skip_last sample"){
    printf("//! [skip_last sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).skip_last(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [skip_last sample]\n");
}
