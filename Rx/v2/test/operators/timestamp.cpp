#include "../test.h"
#include "rxcpp/operators/rx-timestamp.hpp"

using namespace std::chrono;

SCENARIO("should not emit timestamped items if the source never emits any items", "[timestamp][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type::time_point time_point;

        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("timestamp operator is invoked"){

            auto res = w.start(
                [xs]() {
                    return xs
                            | rxo::timestamp();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<std::pair<int, time_point>>::recorded_type>();
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

SCENARIO("should not emit timestamped items if the source observable is empty", "[timestamp][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type::time_point time_point;

        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::pair<int, time_point>> on_timestamp;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("timestamp operator is invoked"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timestamp();
                }
            );

            THEN("the output only contains complete message"){
                auto required = rxu::to_vector({
                    on_timestamp.completed(250)
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

SCENARIO("should emit timestamped items for every item in the source observable", "[timestamp][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type clock_type;
        typedef clock_type::time_point time_point;

        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::pair<int, time_point>> on_timestamp;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(250)
        });

        WHEN("timestamp operator is invoked"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timestamp(so);
                }
            );

            THEN("the output contains the emitted items while subscribed"){
                auto required = rxu::to_vector({
                    on_timestamp.next(210, std::make_pair(2, clock_type::time_point(milliseconds(210)))),
                    on_timestamp.next(240, std::make_pair(3, clock_type::time_point(milliseconds(240)))),
                    on_timestamp.completed(250)
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

SCENARIO("should emit timestamped items and an error if there is an error", "[timestamp][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type clock_type;
        typedef clock_type::time_point time_point;

        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::pair<int, time_point>> on_timestamp;

        std::runtime_error ex("on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.error(250, ex)
        });

        WHEN("timestamp operator is invoked"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timestamp(so);
                }
            );

            THEN("the output contains emitted items and an error"){
                auto required = rxu::to_vector({
                    on_timestamp.next(210, std::make_pair(2, clock_type::time_point(milliseconds(210)))),
                    on_timestamp.next(240, std::make_pair(3, clock_type::time_point(milliseconds(240)))),
                    on_timestamp.error(250, ex)
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

SCENARIO("timestamp doesn't provide copies", "[timestamp][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        typedef rxsc::detail::test_type::clock_type clock_type;
        typedef clock_type::time_point time_point;

        auto          empty_on_next = [](std::pair<copy_verifier, time_point>) {};
        auto          sub           = rx::make_observer<std::pair<copy_verifier, time_point>>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable().timestamp();
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to pair
                REQUIRE(verifier.get_copy_count() == 1);
                // 1 move pair to final lambda
                REQUIRE(verifier.get_move_count() == 1);
            }
        }
    }
}


SCENARIO("timestamp doesn't provide copies for move", "[timestamp][operators][copies]"){
    GIVEN("observable and subscriber")
    {
        typedef rxsc::detail::test_type::clock_type clock_type;
        typedef clock_type::time_point time_point;

        auto          empty_on_next = [](std::pair<copy_verifier, time_point>) {};
        auto          sub           = rx::make_observer<std::pair<copy_verifier, time_point>>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable_for_move().timestamp();
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                //  1 move to pair + 1 move of pair to final lambda
                REQUIRE(verifier.get_move_count() == 2);
            }
        }
    }
}
