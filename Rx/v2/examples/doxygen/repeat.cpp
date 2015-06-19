#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("repeat sample"){
    printf("//! [repeat sample]\n");
    auto values = rxcpp::observable<>::from(1, 2).
        repeat().
        take(5);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [repeat sample]\n");
}

SCENARIO("repeat count sample"){
    printf("//! [repeat count sample]\n");
    auto values = rxcpp::observable<>::from(1, 2).repeat(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [repeat count sample]\n");
}

SCENARIO("repeat error sample"){
    printf("//! [repeat error sample]\n");
    auto values = rxcpp::observable<>::from(1, 2).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        repeat();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [repeat error sample]\n");
}
