#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("scan some data with seed", "[scan][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int seed = 1;

        auto xs = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
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
                    on.on_next(210, seed + 2),
                    on.on_next(220, seed + 2 + 3),
                    on.on_next(230, seed + 2 + 3 + 4),
                    on.on_next(240, seed + 2 + 3 + 4 + 5),
                    on.on_completed(250)
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
