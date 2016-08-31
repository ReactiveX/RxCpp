#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("sequence_equal sample"){
    printf("//! [sequence_equal sample]\n");
    auto source = rxcpp::observable<>::range(1, 3);
    auto values = source.sequence_equal(rxcpp::observable<>::range(1, 3));
    values.
        subscribe(
            [](bool v){ printf("OnNext: %s\n", v ? "true" : "false"); },
            [](){ printf("OnCompleted\n");} );
    printf("//! [sequence_equal sample]\n");
}
