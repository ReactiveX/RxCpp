#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("switch_on_next sample"){
    printf("//! [switch_on_next sample]\n");
    auto base = rxcpp::observable<>::interval(std::chrono::milliseconds(30)).
        take(3).
        map([](long){
            return rxcpp::observable<>::interval(std::chrono::milliseconds(10)).as_dynamic();
        });
    auto values = base.switch_on_next().take(10);
    values.
        subscribe(
            [](long v){printf("OnNext: %ld\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [switch_on_next sample]\n");
}

SCENARIO("threaded switch_on_next sample"){
    printf("//! [threaded switch_on_next sample]\n");
    auto base = rxcpp::observable<>::interval(std::chrono::milliseconds(30)).
        take(3).
        map([](long){
            return rxcpp::observable<>::interval(std::chrono::milliseconds(10), rxcpp::observe_on_event_loop()).as_dynamic();
        });
    auto values = base.switch_on_next(rxcpp::observe_on_new_thread()).take(10);
    values.
        as_blocking().
        subscribe(
            [](long v){printf("OnNext: %ld\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [threaded switch_on_next sample]\n");
}
