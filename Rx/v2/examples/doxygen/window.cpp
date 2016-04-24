#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("window count sample"){
    printf("//! [window count sample]\n");
    int counter = 0;
    auto values = rxcpp::observable<>::range(1, 5).window(2);
    values.
        subscribe(
            [&counter](rxcpp::observable<int> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](int v){printf("[window %d] OnNext: %d\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window count sample]\n");
}

SCENARIO("window count+skip sample"){
    printf("//! [window count+skip sample]\n");
    int counter = 0;
    auto values = rxcpp::observable<>::range(1, 7).window(2, 3);
    values.
        subscribe(
            [&counter](rxcpp::observable<int> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](int v){printf("[window %d] OnNext: %d\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window count+skip sample]\n");
}

SCENARIO("window period+skip+coordination sample"){
    printf("//! [window period+skip+coordination sample]\n");
    int counter = 0;
    auto period = std::chrono::milliseconds(4);
    auto skip = std::chrono::milliseconds(6);
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_with_time(period, skip, rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+skip+coordination sample]\n");
}

SCENARIO("window period+skip sample"){
    printf("//! [window period+skip sample]\n");
    int counter = 0;
    auto period = std::chrono::milliseconds(4);
    auto skip = std::chrono::milliseconds(6);
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_with_time(period, skip);
    values.
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+skip sample]\n");
}

SCENARIO("window period+skip overlapping sample"){
    printf("//! [window period+skip overlapping sample]\n");
    int counter = 0;
    auto period = std::chrono::milliseconds(6);
    auto skip = std::chrono::milliseconds(4);
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_with_time(period, skip);
    values.
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+skip overlapping sample]\n");
}

SCENARIO("window period+skip empty sample"){
    printf("//! [window period+skip empty sample]\n");
    int counter = 0;
    auto period = std::chrono::milliseconds(2);
    auto skip = std::chrono::milliseconds(4);
    auto values = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).
        window_with_time(period, skip);
    values.
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+skip empty sample]\n");
}

SCENARIO("window period+coordination sample"){
    printf("//! [window period+coordination sample]\n");
    int counter = 0;
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_with_time(std::chrono::milliseconds(4), rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+coordination sample]\n");
}

SCENARIO("window period sample"){
    printf("//! [window period sample]\n");
    int counter = 0;
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_with_time(std::chrono::milliseconds(4));
    values.
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period sample]\n");
}

SCENARIO("window period+count+coordination sample"){
    printf("//! [window period+count+coordination sample]\n");
    int counter = 0;
    auto int1 = rxcpp::observable<>::range(1L, 3L);
    auto int2 = rxcpp::observable<>::timer(std::chrono::milliseconds(50));
    auto values = int1.
        concat(int2).
        window_with_time_or_count(std::chrono::milliseconds(20), 2, rxcpp::observe_on_event_loop());
    values.
        as_blocking().
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+count+coordination sample]\n");
}

SCENARIO("window period+count sample"){
    printf("//! [window period+count sample]\n");
    int counter = 0;
    auto int1 = rxcpp::observable<>::range(1L, 3L);
    auto int2 = rxcpp::observable<>::timer(std::chrono::milliseconds(50));
    auto values = int1.
        concat(int2).
        window_with_time_or_count(std::chrono::milliseconds(20), 2);
    values.
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window period+count sample]\n");
}

SCENARIO("window toggle+coordination sample"){
    printf("//! [window toggle+coordination sample]\n");
    int counter = 0;
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_toggle(
            rxcpp::observable<>::interval(std::chrono::milliseconds(4)), 
            [](long){
                return rxcpp::observable<>::interval(std::chrono::milliseconds(4)).skip(1);
            },
            rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window toggle+coordination sample]\n");
}

SCENARIO("window toggle sample"){
    printf("//! [window toggle sample]\n");
    int counter = 0;
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        window_toggle(
            rxcpp::observable<>::interval(std::chrono::milliseconds(4)), 
            [](long){
                return rxcpp::observable<>::interval(std::chrono::milliseconds(4)).skip(1);
            });
    values.
        subscribe(
            [&counter](rxcpp::observable<long> v){
                int id = counter++;
                printf("[window %d] Create window\n", id);
                v.subscribe(
                    [id](long v){printf("[window %d] OnNext: %ld\n", id, v);},
                    [id](){printf("[window %d] OnCompleted\n", id);});
            });
    printf("//! [window toggle sample]\n");
}
