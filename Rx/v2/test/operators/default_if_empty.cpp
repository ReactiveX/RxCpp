#include "../test.h"
#include <rxcpp/operators/rx-switch_if_empty.hpp>

SCENARIO("default_if_empty should not switch if the source is not empty", "[default_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(210, 1),
            on.completed(250)
        });

        WHEN("started"){

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::default_if_empty(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output only contains an item from the source") {
                auto required = rxu::to_vector({
                    on.next(210, 1),
                    on.completed(250)
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

SCENARIO("default_if_empty should switch if the source is empty", "[default_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("started"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .default_if_empty(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains an item from the backup source") {
                auto required = rxu::to_vector({
                    on.next(250, 2),
                    on.completed(250)
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

SCENARIO("default_if_empty - never", "[default_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("started"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .default_if_empty(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
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

SCENARIO("default_if_empty - source throws", "[default_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("default_if_empty on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("started"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .default_if_empty(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains an error from the source"){
                auto required = rxu::to_vector({
                    on.error(250, ex)
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

SCENARIO("default_if_empty doesn't provide copies", "[default_if_empty][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable().default_if_empty(copy_verifier{});
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


SCENARIO("default_if_empty doesn't provide copies for move", "[default_if_empty][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable_for_move().default_if_empty(copy_verifier{});
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                // 1 move to final lambda
                REQUIRE(verifier.get_move_count() == 1);
            }
        }
    }
}

