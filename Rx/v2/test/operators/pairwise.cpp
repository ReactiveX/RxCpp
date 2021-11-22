#include "../test.h"
#include "rxcpp/operators/rx-pairwise.hpp"

SCENARIO("pairwise - enough items to create pairs", "[pairwise][operators]") {
    GIVEN("a cold observable of n ints") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::tuple<int, int>> on_pairwise;

        auto xs = sc.make_cold_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.next(350, 5),
            on.completed(400),
        });

        WHEN("taken pairwise") {

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::pairwise()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains n-1 tuples of ints"){
                auto delay = rxcpp::schedulers::test::subscribed_time;
                auto required = rxu::to_vector({
                    on_pairwise.next(210 + delay, std::make_tuple(1, 2)),
                    on_pairwise.next(240 + delay, std::make_tuple(2, 3)),
                    on_pairwise.next(290 + delay, std::make_tuple(3, 4)),
                    on_pairwise.next(350 + delay, std::make_tuple(4, 5)),
                    on_pairwise.completed(400 + delay)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("pairwise - not enough items to create a pair", "[pairwise][operators]") {
    GIVEN("a cold observable of 1 ints") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::tuple<int, int>> on_pairwise;

        auto xs = sc.make_cold_observable({
            on.next(180, 1),
            on.completed(400),
        });

        WHEN("taken pairwise") {

            auto res = w.start(
                [xs]() {
                    return xs
                        .pairwise()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains no tuples of ints"){
                auto delay = rxcpp::schedulers::test::subscribed_time;
                auto required = rxu::to_vector({
                    on_pairwise.completed(400 + delay)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("pairwise doesn't provide copies", "[pairwise][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](std::tuple<copy_verifier, copy_verifier>) {};
        auto          sub           = rx::make_observer<std::tuple<copy_verifier, copy_verifier>>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable(2).pairwise();
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to internal state for first, 2 copies for second (one in result, one in internal state)
                REQUIRE(verifier.get_copy_count() == 3);
                 // 1 move to final tuple for first, 1 move per object to final lambda
                REQUIRE(verifier.get_move_count() == 3);
            }
        }
    }
}


SCENARIO("pairwise doesn't provide copies for move", "[pairwise][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](std::tuple<copy_verifier, copy_verifier>) {};
        auto          sub           = rx::make_observer<std::tuple<copy_verifier, copy_verifier>>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable_for_move(2).pairwise();
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to tuple for second object
                REQUIRE(verifier.get_copy_count() == 1);
                // 1 move to internal state per object, 1 move from state for first + move tuple
                REQUIRE(verifier.get_move_count() == 5);
            }
        }
    }
}

