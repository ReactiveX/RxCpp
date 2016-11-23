#include "../test.h"
#include <rxcpp/operators/rx-all.hpp>

SCENARIO("all emits true if every item emitted by the source observable evaluated as true", "[all][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_all;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 2),
            on.completed(250)
        });

        WHEN("a predicate function is passed to the all operator") {

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::all([](int n) { return n == 2; })
                        | rxo::as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true") {
                auto required = rxu::to_vector({
                    on_all.next(250, true),
                    on_all.completed(250)
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

SCENARIO("all emits false if any item emitted by the source observable evaluated as false", "[all][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_all;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.completed(250)
        });

        WHEN("a predicate function is passed to the all operator") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .all([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013

                }
            );

            THEN("the output only contains false") {
                auto required = rxu::to_vector({
                    on_all.next(220, false),
                    on_all.completed(220)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("all emits true if the source observable is empty", "[all][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_all;

        auto xs = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("a predicate function is passed to the all operator") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .all([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true") {
                auto required = rxu::to_vector({
                    on_all.next(250, true),
                    on_all.completed(250)
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

SCENARIO("all never emits if the source observable never emits any items", "[all][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_all;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("a predicate function is passed to the all operator") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .all([](int n) { return n == 2; })
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

SCENARIO("all emits an error if the source observable emit an error", "[all][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_all;

        std::runtime_error ex("all on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("a predicate function is passed to the all operator") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .all([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains an error") {
                auto required = rxu::to_vector({
                    on_all.error(250, ex)
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