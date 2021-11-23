#include "../test.h"
#include <rxcpp/operators/rx-any.hpp>

// NOTE: `exists` is an alias of `any`

SCENARIO("exists emits true if an item satisfies the given condition", "[exists][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_exists;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::exists([](int n) { return n == 2; })
                        | rxo::as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_exists.next(210, true),
                    on_exists.completed(210)
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

SCENARIO("exists emits false if no item satisfies the given condition", "[exists][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_exists;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .exists([](int n) { return n > 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013

                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_exists.next(250, false),
                    on_exists.completed(250)
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

SCENARIO("exists emits false if the source observable is empty", "[exists][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_exists;

        auto xs = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .exists([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_exists.next(250, false),
                    on_exists.completed(250)
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
SCENARIO("exists never emits if the source observable never emits any items", "[exists][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_exists;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .exists([](int n) { return n == 2; })
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

SCENARIO("exists emits an error", "[exists][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_exists;

        std::runtime_error ex("exists on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .exists([](int n) { return n == 2; })
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains only error"){
                auto required = rxu::to_vector({
                    on_exists.error(250, ex)
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


SCENARIO("exists doesn't provide copies", "[exists][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](bool) {};
        auto          sub           = rx::make_observer<bool>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable().exists([](copy_verifier) { return true; });
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to final lambda
                REQUIRE(verifier.get_copy_count() == 1);
                REQUIRE(verifier.get_move_count() == 0);
            }
        }
    }
}

