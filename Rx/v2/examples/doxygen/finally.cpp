#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("finally sample"){
    printf("//! [finally sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        finally([](){
            printf("The final action\n");
        });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [finally sample]\n");
}

SCENARIO("error finally sample"){
    printf("//! [error finally sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        finally([](){
            printf("The final action\n");
        });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](rxcpp::util::error_ptr ep){
                printf("OnError: %s\n", rxcpp::util::what(ep).c_str());
            },
            [](){printf("OnCompleted\n");});
    printf("//! [error finally sample]\n");
}
