#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("from sample"){
    printf("//! [from sample]\n");
    auto values = rxcpp::observable<>::from(1, 2, 3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [from sample]\n");
}

std::string get_pid();

SCENARIO("threaded from sample"){
    printf("//! [threaded from sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto values = rxcpp::observable<>::from(rxcpp::observe_on_new_thread(), 1, 2, 3).map([](int v){
        printf("[thread %s] Emit value: %d\n", get_pid().c_str(), v);
        return v;
    });
    values.
        as_blocking().
        subscribe(
            [](int v){printf("[thread %s] OnNext: %d\n", get_pid().c_str(), v);},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded from sample]\n");
}

