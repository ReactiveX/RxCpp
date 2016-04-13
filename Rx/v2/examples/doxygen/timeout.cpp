#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("timeout sample"){
    printf("//! [timeout sample]\n");

    using namespace std::chrono;
    auto values = rxcpp::observable<>::interval(milliseconds(100))
            .take(3)
            .concat(rxcpp::observable<>::interval(milliseconds(500)))
            .timeout(milliseconds(200));
    values.
        subscribe(
            [](long v) { printf("OnNext: %ld\n", v); },
            [](std::exception_ptr ep) {
                try {
                    std::rethrow_exception(ep);
                } catch (const rxcpp::timeout_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            []() { printf("OnCompleted\n"); });
    printf("//! [timeout sample]\n");
}
