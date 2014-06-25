#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("distinct_until_changed - some changes", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2), //*
            on.on_next(215, 3), //*
            on.on_next(220, 3),
            on.on_next(225, 2), //*
            on.on_next(230, 2),
            on.on_next(230, 1), //*
            on.on_next(240, 2), //*
            on.on_completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2), //*
                    on.on_next(215, 3), //*
                    on.on_next(225, 2), //*
                    on.on_next(230, 1), //*
                    on.on_next(240, 2), //*
                    on.on_completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
