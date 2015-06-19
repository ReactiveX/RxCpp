#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("subscribe by subscriber"){
    printf("//! [subscribe by subscriber]\n");
    auto subscriber = rxcpp::make_subscriber<int>(
        [](int v){printf("OnNext: %d\n", v);},
        [](){printf("OnCompleted\n");});
    auto values = rxcpp::observable<>::range(1, 3);
    values.subscribe(subscriber);
    printf("//! [subscribe by subscriber]\n");
}

SCENARIO("subscribe by observer"){
    printf("//! [subscribe by observer]\n");
    auto subscriber = rxcpp::make_subscriber<int>(
        [](int v){printf("OnNext: %d\n", v);},
        [](){printf("OnCompleted\n");});
    auto values1 = rxcpp::observable<>::range(1, 3);
    auto values2 = rxcpp::observable<>::range(4, 6);
    values1.subscribe(subscriber.get_observer());
    values2.subscribe(subscriber.get_observer());
    printf("//! [subscribe by observer]\n");
}

SCENARIO("subscribe by on_next"){
    printf("//! [subscribe by on_next]\n");
    auto values = rxcpp::observable<>::range(1, 3);
    values.subscribe(
        [](int v){printf("OnNext: %d\n", v);});
    printf("//! [subscribe by on_next]\n");
}

SCENARIO("subscribe by on_next and on_error"){
    printf("//! [subscribe by on_next and on_error]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source")));
    values.subscribe(
        [](int v){printf("OnNext: %d\n", v);},
        [](std::exception_ptr ep){
            try {std::rethrow_exception(ep);}
            catch (const std::exception& ex) {
                printf("OnError: %s\n", ex.what());
            }
        });
    printf("//! [subscribe by on_next and on_error]\n");
}

SCENARIO("subscribe by on_next and on_completed"){
    printf("//! [subscribe by on_next and on_completed]\n");
    auto values = rxcpp::observable<>::range(1, 3);
    values.subscribe(
        [](int v){printf("OnNext: %d\n", v);},
        [](){printf("OnCompleted\n");});
    printf("//! [subscribe by on_next and on_completed]\n");
}

SCENARIO("subscribe by subscription, on_next, and on_completed"){
    printf("//! [subscribe by subscription, on_next, and on_completed]\n");
    auto subscription = rxcpp::composite_subscription();
    auto values = rxcpp::observable<>::range(1, 5);
    values.subscribe(
        subscription,
        [&subscription](int v){
            printf("OnNext: %d\n", v);
            if (v == 3)
                subscription.unsubscribe();
        },
        [](){printf("OnCompleted\n");});
    printf("//! [subscribe by subscription, on_next, and on_completed]\n");
}

SCENARIO("subscribe by on_next, on_error, and on_completed"){
    printf("//! [subscribe by on_next, on_error, and on_completed]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source")));
    values.subscribe(
        [](int v){printf("OnNext: %d\n", v);},
        [](std::exception_ptr ep){
            try {std::rethrow_exception(ep);}
            catch (const std::exception& ex) {
                printf("OnError: %s\n", ex.what());
            }
        },
        [](){printf("OnCompleted\n");});
    printf("//! [subscribe by on_next, on_error, and on_completed]\n");
}

SCENARIO("subscribe unsubscribe"){
    printf("//! [subscribe unsubscribe]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::never<int>()).
        finally([](){printf("The final action\n");});
    auto subscription = values.subscribe(
        [](int v){printf("OnNext: %d\n", v);},
        [](){printf("OnCompleted\n");});
    subscription.unsubscribe();
    printf("//! [subscribe unsubscribe]\n");
}
