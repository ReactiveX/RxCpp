#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

std::string get_pid();

SCENARIO("subscribe_on sample"){
    printf("//! [subscribe_on sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto values = rxcpp::observable<>::range(1, 3).
        map([](int v){
            printf("[thread %s] Emit value %d\n", get_pid().c_str(), v);
            return v;
        });
    values.
        subscribe_on(rxcpp::synchronize_new_thread()).
        as_blocking().
        subscribe(
            [](int v){printf("[thread %s] OnNext: %d\n", get_pid().c_str(), v);},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [subscribe_on sample]\n");
}
