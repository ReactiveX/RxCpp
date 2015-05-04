#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("error sample"){
    printf("//! [error sample]\n");
    auto values = rxcpp::observable<>::error<int>(std::runtime_error("Error from source"));
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
    printf("//! [error sample]\n");
}

SCENARIO("threaded error sample"){
    printf("//! [threaded error sample]\n");
    auto values = rxcpp::observable<>::error<int>(std::runtime_error("Error from source"), rxcpp::observe_on_event_loop());
    values.
        as_blocking().
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [threaded error sample]\n");
}
