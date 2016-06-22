#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("timestamp sample") {
    printf("//! [timestamp sample]\n");

    typedef rxcpp::schedulers::scheduler::clock_type::time_point time_point;

    using namespace std::chrono;
    auto values = rxcpp::observable<>::interval(milliseconds(100))
            .timestamp()
            .take(3);
    time_point start = rxcpp::identity_current_thread().now();
    values.
        subscribe(
            [&](std::pair<long, time_point> v) { 
                long long int ms = duration_cast<milliseconds>(v.second - start).count();
                printf("OnNext: %ld @%lldms\n", v.first, ms); 
            },
            [](std::exception_ptr ep) {
                try {
                    std::rethrow_exception(ep);
                } catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            []() { printf("OnCompleted\n"); });
    printf("//! [timestamp sample]\n");
}

SCENARIO("timestamp operator syntax sample") {
    using namespace rxcpp;
    using namespace rxcpp::sources;
    using namespace rxcpp::operators;
    using namespace std::chrono;

    typedef rxcpp::schedulers::scheduler::clock_type::time_point time_point;

    printf("//! [timestamp operator syntax sample]\n");
    auto values = interval(milliseconds(100))
                  | timestamp()
                  | take(3);
    time_point start = rxcpp::identity_current_thread().now();
    values.
            subscribe(
            [&](std::pair<long, time_point> v) { 
                long long int ms = duration_cast<milliseconds>(v.second - start).count();
                printf("OnNext: %ld @%lldms\n", v.first, ms); 
            },
            [](std::exception_ptr ep) {
                try {
                    std::rethrow_exception(ep);
                } catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            []() { printf("OnCompleted\n"); });
    printf("//! [timestamp operator syntax sample]\n");
}