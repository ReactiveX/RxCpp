#include "../test.h"

SCENARIO("reduce some data with seed", "[reduce][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 42;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 0),
            on.next(220, 1),
            on.next(230, 2),
            on.next(240, 3),
            on.next(250, 4),
            on.completed(260)
        });

        auto sum = xs.sum();

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs
                        .reduce(seed,
                            [](int sum, int x) {
                                return sum + x;
                            },
                            [](int sum) {
                                return sum * 5;
                            })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(260, (seed + 0 + 1 + 2 + 3 + 4) * 5),
                    on.completed(260)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 260)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("average some data", "[reduce][average][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<double> d_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 3),
            on.next(220, 4),
            on.next(230, 2),
            on.completed(250)
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [&]() {
                    return xs.average();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    d_on.next(250, 3.0),
                    d_on.completed(250)
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
