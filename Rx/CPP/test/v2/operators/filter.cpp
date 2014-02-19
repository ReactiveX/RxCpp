
#define RXCPP_USE_OBSERVABLE_MEMBERS 1
#define RXCPP_SUBJECT_TEST_ASYNC 0

#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxo=rxcpp::operators;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;
namespace rxsub=rxcpp::subjects;
namespace rxn=rxcpp::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt=rxcpp::test;

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

SCENARIO("subject test", "[subject][subjects]"){
    GIVEN("a subject"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            const int onnextcalls = 10000000;

            {
                int c = 0;
                int n = 1;
                auto o = rx::make_observer<int>([&c](int){++c;});
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop no subject     : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                int c = 0;
                int n = 1;
                auto start = clock::now();
                rxs::range<int>(0, onnextcalls).subscribe([&c](int){++c;});
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "range no subject    : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                std::recursive_mutex m;
                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    std::unique_lock<std::recursive_mutex> guard(m);
                    ++c;
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop recursive_mutex: " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            for (int n = 0; n < 10; n++)
            {
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, c]() {
                        auto source = sub.get_observable();
                        auto subscription = sub.get_observer();
                        while(subscription.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            rx::composite_subscription cs;
                            assert(cs.is_subscribed());
                            source.subscribe(cs, [c, cs](int){
                                ++(*c);
                                cs.unsubscribe();
                            });
                        }
                        return 0;
                    });
#else
                    sub.get_observable().subscribe([c](int){++(*c);});
#endif
                }

                auto o = sub.get_observer();

                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop                : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            for (int n = 0; n < 10; n++)
            {
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, c]() {
                        while(sub.get_observer().is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            rx::composite_subscription cs;
                            sub.get_observable().subscribe(cs, [c, cs](int){
                                ++(*c);
                                cs.unsubscribe();
                            });
                        }
                        return 0;
                    });
#else
                    sub.get_observable().subscribe([c](int){++(*c);});
#endif
                }

                auto o = sub.get_observer();

                auto start = clock::now();
                rxs::range<int>(0, onnextcalls).subscribe(o);
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "range               : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }
        }
    }
}

SCENARIO("filter stops on completion", "[filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        long invoked = 0;

        m::recorded_type messages[] = {
            m::on_next(110, 1),
            m::on_next(180, 2),
            m::on_next(230, 3),
            m::on_next(270, 4),
            m::on_next(340, 5),
            m::on_next(380, 6),
            m::on_next(390, 7),
            m::on_next(450, 8),
            m::on_next(470, 9),
            m::on_next(560, 10),
            m::on_next(580, 11),
            m::on_completed(600),
            m::on_next(610, 12),
            m::on_error(620, std::exception()),
            m::on_completed(630)
        };
        auto xs = sc->make_hot_observable(messages);

        WHEN("filtered to ints that are primes"){
            auto res = sc->start<int>(
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
                }
            );
            THEN("the output only contains primes"){
                m::recorded_type items[] = {
                    m::on_next(230, 3),
                    m::on_next(340, 5),
                    m::on_next(390, 7),
                    m::on_next(580, 11),
                    m::on_completed(600)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rxn::subscription items[] = {
                    m::subscribe(200, 600)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("filter was called until completed"){
                REQUIRE(9 == invoked);
            }
        }
    }
}


SCENARIO("where stops on disposal", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        long invoked = 0;

        m::recorded_type messages[] = {
            m::on_next(110, 1),
            m::on_next(180, 2),
            m::on_next(230, 3),
            m::on_next(270, 4),
            m::on_next(340, 5),
            m::on_next(380, 6),
            m::on_next(390, 7),
            m::on_next(450, 8),
            m::on_next(470, 9),
            m::on_next(560, 10),
            m::on_next(580, 11),
            m::on_completed(600)
        };
        auto xs = sc->make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = sc->start<int>(
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
                m::recorded_type items[] = {
                    m::on_next(230, 3),
                    m::on_next(340, 5),
                    m::on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rxn::subscription items[] = {
                    m::subscribe(200, 400)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

SCENARIO("where stops on error", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        long invoked = 0;

        std::exception ex;

        auto xs = sc->make_hot_observable(
            [ex]() {
                m::recorded_type messages[] = {
                    m::on_next(110, 1),
                    m::on_next(180, 2),
                    m::on_next(230, 3),
                    m::on_next(270, 4),
                    m::on_next(340, 5),
                    m::on_next(380, 6),
                    m::on_next(390, 7),
                    m::on_next(450, 8),
                    m::on_next(470, 9),
                    m::on_next(560, 10),
                    m::on_next(580, 11),
                    m::on_error(600, ex),
                    m::on_next(610, 12),
                    m::on_error(620, std::exception()),
                    m::on_completed(630)
                };
                return rxu::to_vector(messages);
            }()
            );

        WHEN("filtered to ints that are primes"){

            auto res = sc->start<int>(
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
                m::recorded_type items[] = {
                    m::on_next(230, 3),
                    m::on_next(340, 5),
                    m::on_next(390, 7),
                    m::on_next(580, 11),
                    m::on_error(600, ex),
                };
                auto required = rxu::to_vector(items);
                auto actual = res.messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rxn::subscription items[] = {
                    m::subscribe(200, 600)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(9 == invoked);
            }
        }
    }
}

SCENARIO("where stops on throw from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        long invoked = 0;

        std::exception ex;

        auto xs = sc->make_hot_observable(
            []() {
                m::recorded_type messages[] = {
                    m::on_next(110, 1),
                    m::on_next(180, 2),
                    m::on_next(230, 3),
                    m::on_next(270, 4),
                    m::on_next(340, 5),
                    m::on_next(380, 6),
                    m::on_next(390, 7),
                    m::on_next(450, 8),
                    m::on_next(470, 9),
                    m::on_next(560, 10),
                    m::on_next(580, 11),
                    m::on_completed(600),
                    m::on_next(610, 12),
                    m::on_error(620, std::exception()),
                    m::on_completed(630)
                };
                return rxu::to_vector(messages);
            }()
            );

        WHEN("filtered to ints that are primes"){

            auto res = sc->start<int>(
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
                m::recorded_type items[] = {
                    m::on_next(230, 3),
                    m::on_next(340, 5),
                    m::on_error(380, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rxn::subscription items[] = {
                    m::subscribe(200, 380)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(4 == invoked);
            }
        }
    }
}

SCENARIO("where stops on dispose from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        long invoked = 0;

        auto xs = sc->make_hot_observable(
            []() {
                m::recorded_type messages[] = {
                    m::on_next(110, 1),
                    m::on_next(180, 2),
                    m::on_next(230, 3),
                    m::on_next(270, 4),
                    m::on_next(340, 5),
                    m::on_next(380, 6),
                    m::on_next(390, 7),
                    m::on_next(450, 8),
                    m::on_next(470, 9),
                    m::on_next(560, 10),
                    m::on_next(580, 11),
                    m::on_completed(600),
                    m::on_next(610, 12),
                    m::on_error(620, std::exception()),
                    m::on_completed(630)
                };
                return rxu::to_vector(messages);
            }()
        );

        auto res = sc->make_observer<int>();

        rx::observable<int, rx::dynamic_observable<int>> ys;

        WHEN("filtered to ints that are primes"){

            sc->schedule_absolute(rxsc::test::created_time,
                [&invoked, &res, &ys, &xs](rxsc::action, rxsc::scheduler) {
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
                    return rxsc::make_action_empty();
                });

            sc->schedule_absolute(rxsc::test::subscribed_time, [&ys, &res](rxsc::action, rxsc::scheduler) {
                ys.subscribe(res);
                return rxsc::make_action_empty();
            });

            sc->schedule_absolute(rxsc::test::unsubscribed_time, [&res](rxsc::action, rxsc::scheduler) {
                res.unsubscribe();
                return rxsc::make_action_empty();
            });

            sc->start();

            THEN("the output only contains primes"){
                m::recorded_type items[] = {
                    m::on_next(230, 3),
                    m::on_next(340, 5),
                    m::on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rxn::subscription items[] = {
                    m::subscribe(200, 450)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(6 == invoked);
            }
        }
    }
}
