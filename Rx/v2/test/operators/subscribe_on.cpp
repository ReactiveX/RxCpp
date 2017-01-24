#include "../test.h"
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-subscribe_on.hpp>
#include <rxcpp/operators/rx-observe_on.hpp>

static const int static_subscriptions = 50000;

SCENARIO("for loop subscribes to map with subscribe_on and observe_on", "[hide][for][just][subscribe][subscribe_on][observe_on][long][perf]"){
    const int& subscriptions = static_subscriptions;
    GIVEN("a for loop"){
        WHEN("subscribe 50K times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int runs = 10;

            for (;runs > 0; --runs) {

                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < subscriptions; ++i) {
                    c += rx::observable<>::just(1)
                        .map([](int i) {
                            std::stringstream serializer;
                            serializer << i;
                            return serializer.str();
                        })
                        .map([](const std::string& s) {
                            int i;
                            std::stringstream(s) >> i;
                            return i;
                        })
                        .subscribe_on(rx::observe_on_event_loop())
                        .observe_on(rx::observe_on_event_loop())
                        .as_blocking()
                        .count();
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                REQUIRE(subscriptions == c);
                std::cout << "loop subscribe map subscribe_on observe_on : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
            }
        }
    }
}

SCENARIO("for loop subscribes to map with subscribe_on", "[hide][subscribe_on_only][for][just][subscribe][subscribe_on][long][perf]"){
    const int& subscriptions = static_subscriptions;
    GIVEN("a for loop"){
        WHEN("subscribe 50K times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int runs = 10;

            for (;runs > 0; --runs) {

                int c = 0;
                int n = 1;
                auto start = clock::now();

                for (int i = 0; i < subscriptions; ++i) {
                    c += rx::observable<>::
                        just(1).
                        map([](int i) {
                            std::stringstream serializer;
                            serializer << i;
                            return serializer.str();
                        }).
                        map([](const std::string& s) {
                            int i;
                            std::stringstream(s) >> i;
                            return i;
                        }).
                        subscribe_on(rx::observe_on_event_loop()).
                        as_blocking().
                        count();
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                REQUIRE(subscriptions == c);
                std::cout << "loop subscribe map subscribe_on : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
            }
        }
    }
}

SCENARIO("subscribe_on", "[subscribe][subscribe_on]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(300)
        });

        WHEN("subscribe_on is specified"){

            auto res = w.start(
                [so, xs]() {
                    return xs
                         .subscribe_on(so);
                }
            );

            THEN("the output contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.completed(300)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(201, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("stream subscribe_on", "[subscribe][subscribe_on]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(300)
        });

        WHEN("subscribe_on is specified"){

            auto res = w.start(
                [so, xs]() {
                    return xs
                         | rxo::subscribe_on(so);
                }
            );

            THEN("the output contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.completed(300)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(201, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
