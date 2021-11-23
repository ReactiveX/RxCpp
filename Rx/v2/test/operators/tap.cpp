#include "../test.h"
#include <rxcpp/operators/rx-tap.hpp>

SCENARIO("tap stops on completion", "[tap][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.next(350, 5),
            on.completed(400),
            on.next(410, -1),
            on.completed(420),
            on.error(430, std::runtime_error("error on unsubscribed stream"))
        });

        WHEN("on_next is tapped"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        | rxo::tap([&invoked](int) {
                            invoked++;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(290, 4),
                    on.next(350, 5),
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("tap on_next was called until completed"){
                REQUIRE(4 == invoked);
            }
        }
    }
}

SCENARIO("tap stops on error", "[tap][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        std::runtime_error ex("tap on_error from source");
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.error(300, ex),
            on.next(350, 5),
            on.completed(400),
            on.next(410, -1),
            on.completed(420),
            on.error(430, std::runtime_error("error on unsubscribed stream"))
        });

        WHEN("on_error is tapped"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        .tap([&invoked](rxu::error_ptr) {
                            invoked++;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(290, 4),
                    on.error(300, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("tap on_next was called until completed"){
                REQUIRE(1 == invoked);
            }
        }
    }
}

SCENARIO("tap doesn't provide copies", "[tap][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable().tap([](const auto&){});
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


SCENARIO("tap doesn't provide copies for move", "[tap][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable_for_move().tap([](const auto&){});
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                //  1 move to final lambda
                REQUIRE(verifier.get_move_count() == 1);
            }
        }
    }
}
