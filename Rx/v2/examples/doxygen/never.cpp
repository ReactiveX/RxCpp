#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("never sample"){
    printf("//! [never sample]\n");
    auto values = rxcpp::observable<>::never<int>();
    values.
        take_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(10)).
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [never sample]\n");
}
