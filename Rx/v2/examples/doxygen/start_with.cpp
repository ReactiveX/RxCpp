#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("full start_with sample"){
    printf("//! [full start_with sample]\n");
    auto observable = rxcpp::observable<>::range(10, 12);
    auto values = rxcpp::observable<>::start_with(observable, 1, 2, 3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [full start_with sample]\n");
}

SCENARIO("short start_with sample"){
    printf("//! [short start_with sample]\n");
    auto values = rxcpp::observable<>::range(10, 12).
        start_with(1, 2, 3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [short start_with sample]\n");
}
