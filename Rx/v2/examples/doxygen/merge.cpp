#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("merge sample"){
    printf("//! [merge sample]\n");
    auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(15)).map([](int) {return 1;});
    auto o2 = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).map([](int) {return 2;});
    auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(5)).map([](int) {return 3;});
    auto values = o1.merge(o2, o3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [merge sample]\n");
}

SCENARIO("implicit merge sample"){
    printf("//! [implicit merge sample]\n");
    auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(15)).map([](int) {return 1;});
    auto o2 = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).map([](int) {return 2;});
    auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(5)).map([](int) {return 3;});
    auto base = rxcpp::observable<>::from(o1.as_dynamic(), o2, o3);
    auto values = base.merge();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [implicit merge sample]\n");
}

std::string get_pid();

SCENARIO("threaded merge sample"){
    printf("//! [threaded merge sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).map([](int) {
        printf("[thread %s] Timer1 fired\n", get_pid().c_str());
        return 1;
    });
    auto o2 = rxcpp::observable<>::timer(std::chrono::milliseconds(20)).map([](int) {
        printf("[thread %s] Timer2 fired\n", get_pid().c_str());
        return 2;
    });
    auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(30)).map([](int) {
        printf("[thread %s] Timer3 fired\n", get_pid().c_str());
        return 3;
    });
    auto values = o1.merge(rxcpp::observe_on_new_thread(), o2, o3);
    values.
        as_blocking().
        subscribe(
            [](int v){printf("[thread %s] OnNext: %d\n", get_pid().c_str(), v);},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded merge sample]\n");
}

SCENARIO("threaded implicit merge sample"){
    printf("//! [threaded implicit merge sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).map([](int) {
        printf("[thread %s] Timer1 fired\n", get_pid().c_str());
        return 1;
    });
    auto o2 = rxcpp::observable<>::timer(std::chrono::milliseconds(20)).map([](int) {
        printf("[thread %s] Timer2 fired\n", get_pid().c_str());
        return 2;
    });
    auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(30)).map([](int) {
        printf("[thread %s] Timer3 fired\n", get_pid().c_str());
        return 3;
    });
    auto base = rxcpp::observable<>::from(o1.as_dynamic(), o2, o3);
    auto values = base.merge(rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [](int v){printf("[thread %s] OnNext: %d\n", get_pid().c_str(), v);},
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded implicit merge sample]\n");
}
