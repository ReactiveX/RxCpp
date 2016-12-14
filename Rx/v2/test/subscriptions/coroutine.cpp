#include "../test.h"

#include <rxcpp/rx-coroutine.hpp>

#ifdef _RESUMABLE_FUNCTIONS_SUPPORTED

SCENARIO("coroutine completes", "[coroutine]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(210, 2),
            on.next(310, 10),
            on.completed(350)
        });

        WHEN("for co_await"){

            std::vector<typename rxsc::test::messages<int>::recorded_type> messages;

            w.advance_to(rxsc::test::subscribed_time);

            auto d = [&]() -> std::future<void> {
                try {
                    for co_await (auto n : xs | rxo::as_dynamic()) {
                        messages.push_back(on.next(w.clock(), n));
                    }
                    messages.push_back(on.completed(w.clock()));
                } catch (...) {
                    messages.push_back(on.error(w.clock(), std::current_exception()));
                }
            }();

            w.advance_to(rxsc::test::unsubscribed_time);

            THEN("the function completed"){
                REQUIRE(d.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
            }

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(310, 10),
                    on.completed(350)
                });
                auto actual = messages;
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 350)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("coroutine errors", "[coroutine]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("error in source");

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(210, 2),
            on.error(310, ex),
            on.next(310, 10),
            on.completed(350)
        });

        WHEN("for co_await"){

            std::vector<typename rxsc::test::messages<int>::recorded_type> messages;

            w.advance_to(rxsc::test::subscribed_time);

            auto d = [&]() -> std::future<void> {
                try {
                    for co_await (auto n : xs | rxo::as_dynamic()) {
                        messages.push_back(on.next(w.clock(), n));
                    }
                    messages.push_back(on.completed(w.clock()));
                } catch (...) {
                    messages.push_back(on.error(w.clock(), std::current_exception()));
                }
            }();

            w.advance_to(rxsc::test::unsubscribed_time);

            THEN("the function completed"){
                REQUIRE(d.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
            }

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.error(310, ex)
                });
                auto actual = messages;
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 310)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

#endif
