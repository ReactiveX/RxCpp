#include "../test.h"
#include <rxcpp/operators/rx-take.hpp>
#include <rxcpp/operators/rx-observe_on.hpp>

const int static_onnextcalls = 100000;

SCENARIO("range observed on new_thread", "[hide][range][observe_on_debug][observe_on][long][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a range"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto el = rx::observe_on_new_thread();

            for (int n = 0; n < 10; n++)
            {
                std::atomic_bool disposed;
                std::atomic_bool done;
                auto c = std::make_shared<int>(0);

                rx::composite_subscription cs;
                cs.add([&](){
                    if (!done) {abort();}
                    disposed = true;
                });

                auto start = clock::now();
                rxs::range<int>(1)
                    .take(onnextcalls)
                    .observe_on(el)
                    .as_blocking()
                    .subscribe(
                        cs,
                        [c](int){
                           ++(*c);
                        },
                        [&](){
                            done = true;
                        });
                auto expected = onnextcalls;
                REQUIRE(*c == expected);
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "range -> observe_on new_thread : " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed, int-per-second " << *c / (msElapsed.count() / 1000.0) << std::endl;
            }
        }
    }
}

SCENARIO("observe_on", "[observe][observe_on]"){
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
                         .observe_on(so);
                }
            );

            THEN("the output contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.completed(301)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("stream observe_on", "[observe][observe_on]"){
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

        WHEN("observe_on is specified"){

            auto res = w.start(
                [so, xs]() {
                    return xs
                         | rxo::observe_on(so);
                }
            );

            THEN("the output contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.completed(301)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}