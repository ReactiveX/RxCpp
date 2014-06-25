#define RXCPP_USE_OBSERVABLE_MEMBERS 1

#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxo=rxcpp::operators;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

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
            on.on_next(110, 1),
            on.on_next(180, 2),
            on.on_next(230, 3),
            on.on_next(270, 4),
            on.on_next(340, 5),
            on.on_next(380, 6),
            on.on_next(390, 7),
            on.on_next(450, 8),
            on.on_next(470, 9),
            on.on_next(560, 10),
            on.on_next(580, 11),
            on.on_completed(600),
            on.on_next(610, 12),
            on.on_error(620, std::runtime_error("error in unsubscribed stream")),
            on.on_completed(630)
        });

        WHEN("filtered to ints that are primes"){
            auto res = w.start(
                [&xs, &invoked]() {
#if 0 && RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // demonstrates insertion of user definied operator
                        >> [](rx::observable<int> o)->rx::observable<int>{
                            return rxo::filter([](int){return true;})(o);
                        }
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                }
            );
            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.on_next(230, 3),
                    on.on_next(340, 5),
                    on.on_next(390, 7),
                    on.on_next(580, 11),
                    on.on_completed(600)
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
            on.on_next(110, 1),
            on.on_next(180, 2),
            on.on_next(230, 3),
            on.on_next(270, 4),
            on.on_next(340, 5),
            on.on_next(380, 6),
            on.on_next(390, 7),
            on.on_next(450, 8),
            on.on_next(470, 9),
            on.on_next(560, 10),
            on.on_next(580, 11),
            on.on_completed(600)
		});

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [&xs, &invoked]() {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                auto required = rxu::to_vector({
                    on.on_next(230, 3),
                    on.on_next(340, 5),
                    on.on_next(390, 7)
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
            on.on_next(110, 1),
            on.on_next(180, 2),
            on.on_next(230, 3),
            on.on_next(270, 4),
            on.on_next(340, 5),
            on.on_next(380, 6),
            on.on_next(390, 7),
            on.on_next(450, 8),
            on.on_next(470, 9),
            on.on_next(560, 10),
            on.on_next(580, 11),
            on.on_error(600, ex),
            on.on_next(610, 12),
            on.on_error(620, std::runtime_error("error in unsubscribed stream")),
            on.on_completed(630)
		});

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [xs, &invoked]() {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    return xs
                        .filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
#else
                    return xs
                        >> rxo::filter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                }
            );

            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.on_next(230, 3),
                    on.on_next(340, 5),
                    on.on_next(390, 7),
                    on.on_next(580, 11),
                    on.on_error(600, ex),
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
            on.on_next(110, 1),
            on.on_next(180, 2),
            on.on_next(230, 3),
            on.on_next(270, 4),
            on.on_next(340, 5),
            on.on_next(380, 6),
            on.on_next(390, 7),
            on.on_next(450, 8),
            on.on_next(470, 9),
            on.on_next(560, 10),
            on.on_next(580, 11),
            on.on_completed(600),
            on.on_next(610, 12),
            on.on_error(620, std::runtime_error("error in unsubscribed stream")),
            on.on_completed(630)
		});

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [ex, xs, &invoked]() {
#if RXCPP_USE_OBSERVABLE_MEMBERS
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
#else
                    return xs
                        >> rxo::filter([ex, &invoked](int x) {
                            invoked++;
                            if (x > 5) {
                                throw ex;
                            }
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
#endif
                }
            );

            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.on_next(230, 3),
                    on.on_next(340, 5),
                    on.on_error(380, ex)
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
            on.on_next(110, 1),
            on.on_next(180, 2),
            on.on_next(230, 3),
            on.on_next(270, 4),
            on.on_next(340, 5),
            on.on_next(380, 6),
            on.on_next(390, 7),
            on.on_next(450, 8),
            on.on_next(470, 9),
            on.on_next(560, 10),
            on.on_next(580, 11),
            on.on_completed(600),
            on.on_next(610, 12),
            on.on_error(620, std::exception()),
            on.on_completed(630)
		});

        auto res = w.make_subscriber<int>();

        rx::observable<int, rx::dynamic_observable<int>> ys;

        WHEN("filtered to ints that are primes"){

            w.schedule_absolute(rxsc::test::created_time,
                [&invoked, &res, &ys, &xs](const rxsc::schedulable& scbl) {
#if RXCPP_USE_OBSERVABLE_MEMBERS
                    ys = xs
                        .filter([&invoked, &res](int x) {
                            invoked++;
                            if (x == 8)
                                res.unsubscribe();
                            return IsPrime(x);
                        });
#else
                    ys = xs
                        >> rxo::filter([&invoked, &res](int x) {
                            invoked++;
                            if (x == 8)
                                res.unsubscribe();
                            return IsPrime(x);
                        });
#endif
                });

            w.schedule_absolute(rxsc::test::subscribed_time, [&ys, &res](const rxsc::schedulable& scbl) {
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time, [&res](const rxsc::schedulable& scbl) {
                res.unsubscribe();
            });

            w.start();

            THEN("the output only contains primes"){
                auto required = rxu::to_vector({
                    on.on_next(230, 3),
                    on.on_next(340, 5),
                    on.on_next(390, 7)
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
