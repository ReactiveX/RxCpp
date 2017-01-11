#include "../test.h"
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-take.hpp>
#include <rxcpp/operators/rx-scan.hpp>

SCENARIO("scan: issue 41", "[scan][operators][issue][hide]"){
    GIVEN("map of scan of interval"){
        auto sc = rxsc::make_current_thread();
        auto so = rxcpp::synchronize_in_one_worker(sc);
        auto start = sc.now() + std::chrono::seconds(2);
        auto period = std::chrono::seconds(1);

        rxcpp::observable<>::interval(start, period, so)
            .scan(0, [] (int a, int i) { return a + i; })
            .map([] (int i) { return i * i; })
            .take(10)
            .subscribe([] (int i) { std::cout << i << std::endl; });

    }
}

SCENARIO("scan: seed, never", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::scan(seed, [](int sum, int x) {
                            return sum + x;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("scan: seed, empty", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        .scan(seed, [](int sum, int x) {
                            return sum + x;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("scan: seed, return", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.completed(250)
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        .scan(seed, [](int sum, int x) {
                            return sum + x;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(220, seed + 2),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("scan: seed, throw", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        std::runtime_error ex("scan on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        .scan(seed, [](int sum, int x) {
                            return sum + x;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on error"){
                auto required = rxu::to_vector({
                    on.error(250, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("scan: seed, some data", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        .scan(seed, [](int sum, int x) {
                            return sum + x;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(210, seed + 2),
                    on.next(220, seed + 2 + 3),
                    on.next(230, seed + 2 + 3 + 4),
                    on.next(240, seed + 2 + 3 + 4 + 5),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("scan: seed, accumulator throws", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        std::runtime_error ex("scan on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        .scan(seed, [&](int sum, int x) {
                            if (x == 4) {
                                throw ex;
                            }
                            return sum + x;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on error"){
                auto required = rxu::to_vector({
                    on.next(210, seed + 2),
                    on.next(220, seed + 2 + 3),
                    on.error(230, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
