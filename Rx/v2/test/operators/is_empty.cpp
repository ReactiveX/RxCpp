#include "../test.h"
#include <rxcpp/operators/rx-all.hpp>

SCENARIO("is_empty emits false if the source observable is not empty", "[is_empty][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_is_empty;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 10),
            on.next(220, 20),
            on.completed(250)
        });

        WHEN("is_empty is invoked") {

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::is_empty()
                        | rxo::as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true") {
                auto required = rxu::to_vector({
                    on_is_empty.next(210, false),
                    on_is_empty.completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("is_empty emits true if the source observable is empty", "[is_empty][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_is_empty;

        auto xs = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("is_empty is invoked") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .is_empty()
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true") {
                auto required = rxu::to_vector({
                    on_is_empty.next(250, true),
                    on_is_empty.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("is_empty never emits if the source observable never emits any items", "[is_empty][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_is_empty;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("is_empty is invoked") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .is_empty()
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output is empty") {
                auto required = std::vector<rxsc::test::messages<bool>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("is_empty emits an error if the source observable emit an error", "[is_empty][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_is_empty;

        std::runtime_error ex("is_empty on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("is_empty is invoked") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .is_empty()
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains an error") {
                auto required = rxu::to_vector({
                    on_is_empty.error(250, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
