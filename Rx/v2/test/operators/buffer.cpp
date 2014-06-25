#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("buffer count skip less", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        auto xs = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        WHEN("group each int with the next 2 ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer(3, 1)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.on_next(230, rxu::to_vector(2, 3, 4)),
                    v_on.on_next(240, rxu::to_vector(3, 4, 5)),
                    v_on.on_next(250, rxu::to_vector(4, 5)),
                    v_on.on_next(250, rxu::to_vector(5)),
                    v_on.on_completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
