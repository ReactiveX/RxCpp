#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("map stops on completion", "[map][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.on_next(180, 1),
            on.on_next(210, 2),
            on.on_next(240, 3),
            on.on_next(290, 4),
            on.on_next(350, 5),
            on.on_completed(400),
            on.on_next(410, -1),
            on.on_completed(420),
            on.on_error(430, std::runtime_error("error on unsubscribed stream"))
        });

        WHEN("mapped to ints that are one larger"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        .map([&invoked](int x) {
                            invoked++;
                            return x + 1;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.on_next(210, 3),
                    on.on_next(240, 4),
                    on.on_next(290, 5),
                    on.on_next(350, 6),
                    on.on_completed(400)
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

            THEN("map was called until completed"){
                REQUIRE(4 == invoked);
            }
        }
    }
}
