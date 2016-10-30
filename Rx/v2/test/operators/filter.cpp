#include "../test.h"
#include <rxcpp/operators/rx-filter.hpp>

namespace {
bool IsPrime(int x)
{
    if (x < 2) return false;
    for (int i = 2; i <= x/2; ++i)
    {
        if (x % i == 0)
            return false;
    }
    return true;
}
}

SCENARIO("filter stops on completion", "[filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600),
            on.next(610, 12),
            on.error(620, std::runtime_error("error in unsubscribed stream")),
            on.completed(630)
        });

        WHEN("filtered to ints that are primes"){
            auto res = w.start(
                [&xs, &invoked]() {
                    return xs
                        | rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );
            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7),
                    on.next(580, 11),
                    on.completed(600)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("filter was called until completed"){
                REQUIRE(9 == invoked);
            }
        }
    }
}


SCENARIO("filter stops on disposal", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600)
        });

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [&xs, &invoked]() {
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7)
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

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

SCENARIO("filter stops on error", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        std::runtime_error ex("filter on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.error(600, ex),
            on.next(610, 12),
            on.error(620, std::runtime_error("error in unsubscribed stream")),
            on.completed(630)
        });

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7),
                    on.next(580, 11),
                    on.error(600, ex),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(9 == invoked);
            }
        }
    }
}

SCENARIO("filter stops on throw from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        std::runtime_error ex("filter predicate error");

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600),
            on.next(610, 12),
            on.error(620, std::runtime_error("error in unsubscribed stream")),
            on.completed(630)
        });

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [ex, xs, &invoked]() {
                    return xs
                        .filter([ex, &invoked](int x) {
                            invoked++;
                            if (x > 5) {
                                throw ex;
                            }
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.error(380, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 380)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(4 == invoked);
            }
        }
    }
}

SCENARIO("filter stops on dispose from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600),
            on.next(610, 12),
            on.error(620, std::exception()),
            on.completed(630)
        });

        auto res = w.make_subscriber<int>();

        rx::observable<int, rx::dynamic_observable<int>> ys;

        WHEN("filtered to ints that are primes"){

            w.schedule_absolute(rxsc::test::created_time,
                [&invoked, &res, &ys, &xs](const rxsc::schedulable&) {
                    ys = xs
                        .filter([&invoked, &res](int x) {
                            invoked++;
                            if (x == 8)
                                res.unsubscribe();
                            return IsPrime(x);
                        });
                });

            w.schedule_absolute(rxsc::test::subscribed_time, [&ys, &res](const rxsc::schedulable&) {
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time, [&res](const rxsc::schedulable&) {
                res.unsubscribe();
            });

            w.start();

            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 450)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(6 == invoked);
            }
        }
    }
}
