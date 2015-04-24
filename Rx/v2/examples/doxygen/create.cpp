#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("Create sample"){
    printf("//! [Create sample]\n");
    auto ints = rxcpp::observable<>::create<int>(
        [](rxcpp::subscriber<int> s){
            s.on_next(1);
            s.on_next(2);
            s.on_completed();
    });

    ints.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [Create sample]\n");
}

SCENARIO("Create bad code"){
    printf("//! [Create bad code]\n");
    auto subscription = rxcpp::composite_subscription();
    auto subscriber = rxcpp::make_subscriber<int>(
        subscription,
        [&](int v){
            printf("OnNext: %d\n", v);
            if (v == 2)
                subscription.unsubscribe();
        },
        [](){
            printf("OnCompleted\n");
        });
    rxcpp::observable<>::create<int>(
        [](rxcpp::subscriber<int> s){
            for (int i = 0; i < 5; ++i) {
                s.on_next(i);
                printf("Just sent: OnNext(%d)\n", i);
            }
            s.on_completed();
            printf("Just sent: OnCompleted()\n");
    }).subscribe(subscriber);
    printf("//! [Create bad code]\n");
}

SCENARIO("Create good code"){
    printf("//! [Create good code]\n");
    auto subscription = rxcpp::composite_subscription();
    auto subscriber = rxcpp::make_subscriber<int>(
        subscription,
        [&](int v){
            printf("OnNext: %d\n", v);
            if (v == 2)
                subscription.unsubscribe();
        },
        [](){
            printf("OnCompleted\n");
        });
    rxcpp::observable<>::create<int>(
        [](rxcpp::subscriber<int> s){
            for (int i = 0; i < 5; ++i) {
                if (!s.is_subscribed()) // Stop emitting if nobody is listening
                    break;
                s.on_next(i);
                printf("Just sent: OnNext(%d)\n", i);
            }
            s.on_completed();
            printf("Just sent: OnCompleted()\n");
    }).subscribe(subscriber);
    printf("//! [Create good code]\n");
}

SCENARIO("Create great code"){
    printf("//! [Create great code]\n");
    auto ints = rxcpp::observable<>::create<int>(
        [](rxcpp::subscriber<int> s){
            for (int i = 0; i < 5; ++i) {
                if (!s.is_subscribed()) // Stop emitting if nobody is listening
                    break;
                s.on_next(i);
                printf("Just sent: OnNext(%d)\n", i);
            }
            s.on_completed();
            printf("Just sent: OnCompleted()\n");
    });
    ints.
        take(2).
        subscribe(
            [](int v){
                printf("OnNext: %d\n", v);
            },
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){
                printf("OnCompleted\n");
            });
    printf("//! [Create great code]\n");
}
