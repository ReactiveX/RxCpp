#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("buffer count sample"){
    printf("//! [buffer count sample]\n");
    auto values = rxcpp::observable<>::range(1, 5).buffer(2);
    values.
        subscribe(
            [](std::vector<int> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](int a){
                    printf(" %d", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer count sample]\n");
}

SCENARIO("buffer count+skip sample"){
    printf("//! [buffer count+skip sample]\n");
    auto values = rxcpp::observable<>::range(1, 7).buffer(2, 3);
    values.
        subscribe(
            [](std::vector<int> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](int a){
                    printf(" %d", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer count+skip sample]\n");
}

std::string get_pid();

SCENARIO("buffer period+skip+coordination sample"){
    printf("//! [buffer period+skip+coordination sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto period = std::chrono::milliseconds(4);
    auto skip = std::chrono::milliseconds(6);
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        map([](long v){
            printf("[thread %s] Interval OnNext: %ld\n", get_pid().c_str(), v);
            return v;
        }).
        take(7).
        buffer_with_time(period, skip, rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [](std::vector<long> v){
                printf("[thread %s] OnNext:", get_pid().c_str());
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [buffer period+skip+coordination sample]\n");
}

SCENARIO("buffer period+skip sample"){
    printf("//! [buffer period+skip sample]\n");
    auto period = std::chrono::milliseconds(4);
    auto skip = std::chrono::milliseconds(6);
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        buffer_with_time(period, skip);
    values.
        subscribe(
            [](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period+skip sample]\n");
}

SCENARIO("buffer period+skip overlapping sample"){
    printf("//! [buffer period+skip overlapping sample]\n");
    auto period = std::chrono::milliseconds(6);
    auto skip = std::chrono::milliseconds(4);
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        buffer_with_time(period, skip);
    values.
        subscribe(
            [](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period+skip overlapping sample]\n");
}

SCENARIO("buffer period+skip empty sample"){
    printf("//! [buffer period+skip empty sample]\n");
    auto period = std::chrono::milliseconds(2);
    auto skip = std::chrono::milliseconds(4);
    auto values = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).
        buffer_with_time(period, skip);
    values.
        subscribe(
            [](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period+skip empty sample]\n");
}

SCENARIO("buffer period+coordination sample"){
    printf("//! [buffer period+coordination sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        buffer_with_time(std::chrono::milliseconds(4), rxcpp::observe_on_new_thread());
    values.
        as_blocking().
        subscribe(
            [](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period+coordination sample]\n");
}

SCENARIO("buffer period sample"){
    printf("//! [buffer period sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(1), std::chrono::milliseconds(2)).
        take(7).
        buffer_with_time(std::chrono::milliseconds(4));
    values.
        subscribe(
            [](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period sample]\n");
}

SCENARIO("buffer period+count+coordination sample"){
    printf("//! [buffer period+count+coordination sample]\n");
    auto start = std::chrono::steady_clock::now();
    auto int1 = rxcpp::observable<>::range(1L, 3L);
    auto int2 = rxcpp::observable<>::timer(std::chrono::milliseconds(50));
    auto values = int1.
        concat(int2).
        buffer_with_time_or_count(std::chrono::milliseconds(20), 2, rxcpp::observe_on_event_loop());
    values.
        as_blocking().
        subscribe(
            [start](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period+count+coordination sample]\n");
}

SCENARIO("buffer period+count sample"){
    printf("//! [buffer period+count sample]\n");
    auto start = std::chrono::steady_clock::now();
    auto int1 = rxcpp::observable<>::range(1L, 3L);
    auto int2 = rxcpp::observable<>::timer(std::chrono::milliseconds(50));
    auto values = int1.
        concat(int2).
        buffer_with_time_or_count(std::chrono::milliseconds(20), 2);
    values.
        subscribe(
            [start](std::vector<long> v){
                printf("OnNext:");
                std::for_each(v.begin(), v.end(), [](long a){
                    printf(" %ld", a);
                });
                printf("\n");
            },
            [](){printf("OnCompleted\n");});
    printf("//! [buffer period+count sample]\n");
}
