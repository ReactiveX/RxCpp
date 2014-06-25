#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("combine_latest interleaved with tail", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(215, 2),
            on.on_next(225, 4),
            on.on_completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(220, 3),
            on.on_next(230, 5),
            on.on_next(235, 6),
            on.on_next(240, 7),
            on.on_completed(250)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .combine_latest([](int v2, int v1){return v2 + v1;}, o1)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                auto required = rxu::to_vector({
                    on.on_next(220, 2 + 3),
                    on.on_next(225, 4 + 3),
                    on.on_next(230, 4 + 5),
                    on.on_next(235, 4 + 6),
                    on.on_next(240, 4 + 7),
                    on.on_completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
