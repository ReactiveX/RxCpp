#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("skip_until sample"){
    printf("//! [skip_until sample]\n");
    auto source = rxcpp::observable<>::interval(std::chrono::milliseconds(10)).take(7);
    auto trigger = rxcpp::observable<>::timer(std::chrono::milliseconds(25));
    auto values = source.skip_until(trigger);
    values.
        subscribe(
            [](long v){printf("OnNext: %ld\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [skip_until sample]\n");
}

std::string get_pid();

SCENARIO("threaded skip_until sample"){
    printf("//! [threaded skip_until sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto source = rxcpp::observable<>::interval(std::chrono::milliseconds(10)).take(7).map([](long v){
        printf("[thread %s] Source emits, value = %ld\n", get_pid().c_str(), v);
        return v;
    });
    auto trigger = rxcpp::observable<>::timer(std::chrono::milliseconds(25)).map([](long v){
        printf("[thread %s] Trigger emits, value = %ld\n", get_pid().c_str(), v);
        return v;
    });
    auto values = source.skip_until(trigger, rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [](long v){printf("[thread %s] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded skip_until sample]\n");
}
