#include "../test.h"
#include <rxcpp/operators/rx-any.hpp>

SCENARIO("contains emits true if an item satisfies the given condition", "[contains][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_contains;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::contains(2)
                        | rxo::as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_contains.next(210, true),
                    on_contains.completed(210)
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

SCENARIO("contains emits false if no item satisfies the given condition", "[contains][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_contains;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .contains(20)
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_contains.next(250, false),
                    on_contains.completed(250)
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

SCENARIO("contains emits false if the source observable is empty", "[contains][operators]"){
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_contains;

        auto xs = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .contains(2)
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains true"){
                auto required = rxu::to_vector({
                    on_contains.next(250, false),
                    on_contains.completed(250)
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
SCENARIO("contains never emits if the source observable never emits any items", "[contains][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_contains;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .contains(2)
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

SCENARIO("contains emits an error", "[contains][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> on_contains;

        std::runtime_error ex("contains on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("invoked with a predicate"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .contains(2)
                        .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output only contains only error"){
                auto required = rxu::to_vector({
                    on_contains.error(250, ex)
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

SCENARIO("contains doesn't provide copies", "[contains][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](bool) {};
        auto          sub           = rx::make_observer<bool>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable().contains(copy_verifier{});
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                REQUIRE(verifier.get_move_count() == 0);
            }
        }
    }
}


SCENARIO("contains doesn't provide copies for move", "[contains][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](bool) {};
        auto          sub           = rx::make_observer<bool>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable_for_move().contains(copy_verifier{});
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                REQUIRE(verifier.get_move_count() == 0);
            }
        }
    }
}

SCENARIO("contains provides 1 copy for value to compare", "[contains][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](bool) {};
        auto          sub           = rx::make_observer<bool>(empty_on_next);
        copy_verifier verifier{};
        copy_verifier verifier_to_pass{};
        auto          obs = verifier.get_observable().contains(verifier_to_pass);
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier_to_pass.get_copy_count() == 1);
                REQUIRE(verifier_to_pass.get_move_count() == 0);
            }
        }
    }
}

SCENARIO("contains provides 1 move for value to compare for move", "[contains][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](bool) {};
        auto          sub           = rx::make_observer<bool>(empty_on_next);
        copy_verifier verifier{};
        copy_verifier verifier_to_pass{};
        auto          obs = verifier.get_observable().contains(std::move(verifier_to_pass));
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier_to_pass.get_copy_count() == 0);
                REQUIRE(verifier_to_pass.get_move_count() == 1);
            }
        }
    }
}
