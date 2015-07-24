#include "../test.h"

SCENARIO("map stops on completion", "[map][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.next(350, 5),
            on.completed(400),
            on.next(410, -1),
            on.completed(420),
            on.error(430, std::runtime_error("error on unsubscribed stream"))
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
                    on.next(210, 3),
                    on.next(240, 4),
                    on.next(290, 5),
                    on.next(350, 6),
                    on.completed(400)
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
