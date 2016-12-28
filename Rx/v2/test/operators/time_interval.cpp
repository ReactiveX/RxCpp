#include "../test.h"
#include "rxcpp/operators/rx-time_interval.hpp"

using namespace std::chrono;

SCENARIO("should not emit time intervals if the source never emits any items", "[time_interval][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type::time_point::duration duration;

        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("time_interval operator is invoked"){

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::time_interval();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<duration>::recorded_type>();
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

SCENARIO("should not emit time intervals if the source observable is empty", "[time_interval][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type::time_point::duration duration;

        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<duration> on_time_interval;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("time_interval operator is invoked"){

            auto res = w.start(
                [so, xs]() {
                    return xs.time_interval();
                }
            );

            THEN("the output only contains complete message"){
                auto required = rxu::to_vector({
                    on_time_interval.completed(250)
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

SCENARIO("should emit time intervals for every item in the source observable", "[time_interval][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type clock_type;
        typedef clock_type::time_point::duration duration;

        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<duration> on_time_interval;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(250)
        });

        WHEN("time_interval operator is invoked"){

            auto res = w.start(
                [so, xs]() {
                    return xs.time_interval(so);
                }
            );

            THEN("the output contains the emitted items while subscribed"){
                auto required = rxu::to_vector({
                    on_time_interval.next(210, milliseconds(10)),
                    on_time_interval.next(240, milliseconds(30)),
                    on_time_interval.completed(250)
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

SCENARIO("should emit time intervals and an error if there is an error", "[time_interval][operators]"){
    GIVEN("a source"){
        typedef rxsc::detail::test_type::clock_type clock_type;
        typedef clock_type::time_point::duration duration;

        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<duration> on_time_interval;

        std::runtime_error ex("on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.error(250, ex)
        });

        WHEN("time_interval operator is invoked"){

            auto res = w.start(
                [so, xs]() {
                    return xs.time_interval(so);
                }
            );

            THEN("the output contains emitted items and an error"){
                auto required = rxu::to_vector({
                    on_time_interval.next(210, milliseconds(10)),
                    on_time_interval.next(240, milliseconds(30)),
                    on_time_interval.error(250, ex)
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
