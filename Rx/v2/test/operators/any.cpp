#include "../test.h"
#include <rxcpp/operators/rx-any.hpp>

SCENARIO("any emits true if an item satisfies the given condition", "[any][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_any;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::any([](int n) { return n == 2; })
                        | rxo::as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_any.next(210, true),
                    on_any.completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("any emits false if no item satisfies the given condition", "[any][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_any;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .any([](int n) { return n > 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013

                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_any.next(250, false),
                    on_any.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("any emits false if the source observable is empty", "[any][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_any;

        auto xs = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .any([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_any.next(250, false),
                    on_any.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
SCENARIO("any never emits if the source observable never emits any items", "[any][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_any;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .any([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<bool>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("any emits an error", "[any][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_any;

        std::runtime_error ex("any on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .any([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains only error"){
                auto required = rxu::to_vector({
                    on_any.error(250, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}