#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("time_interval sample") {
    printf("//! [time_interval sample]\n");

    typedef rxcpp::schedulers::scheduler::clock_type::time_point::duration duration_type;

    using namespace std::chrono;
    auto values = rxcpp::observable<>::interval(milliseconds(100))
            .time_interval()
            .take(3);
    values.
        subscribe(
            [&](duration_type v) {
                long long int ms = duration_cast<milliseconds>(v).count();
                printf("OnNext: @%lldms\n", ms);
            },
            [](std::exception_ptr ep) {
                try {
                    std::rethrow_exception(ep);
                } catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            []() { printf("OnCompleted\n"); });
    printf("//! [time_interval sample]\n");
}

SCENARIO("time_interval operator syntax sample") {
    using namespace rxcpp;
    using namespace rxcpp::sources;
    using namespace rxcpp::operators;
    using namespace std::chrono;

    typedef rxcpp::schedulers::scheduler::clock_type::time_point::duration duration_type;

    printf("//! [time_interval operator syntax sample]\n");
    auto values = interval(milliseconds(100))
                  | time_interval()
                  | take(3);
    values.
            subscribe(
            [&](duration_type v) {
                long long int ms = duration_cast<milliseconds>(v).count();
                printf("OnNext: @%lldms\n", ms);
            },
            [](std::exception_ptr ep) {
                try {
                    std::rethrow_exception(ep);
                } catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            []() { printf("OnCompleted\n"); });
    printf("//! [time_interval operator syntax sample]\n");
}
