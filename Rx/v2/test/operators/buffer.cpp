#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxo=rxcpp::operators;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;
namespace rxsub=rxcpp::subjects;
namespace rxn=rxcpp::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt=rxcpp::test;

#include "catch.hpp"

SCENARIO("buffer count skip less", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::vector<int>> mv;
        typedef rxn::subscription life;

        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        typedef mv::recorded_type vrecord;
        auto von_next = mv::on_next;
        auto von_completed = mv::on_completed;
        auto vsubscribe = mv::subscribe;

        record messages1[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto xs = sc.make_hot_observable(messages1);

        WHEN("group each int with the next 2 ints"){

            auto res = w.start<std::vector<int>>(
                [&]() {
                    return xs
                        .buffer(3, 1)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                vrecord items[] = {
                    von_next(230, rxu::to_vector(2, 3, 4)),
                    von_next(240, rxu::to_vector(3, 4, 5)),
                    von_next(250, rxu::to_vector(4, 5)),
                    von_next(250, rxu::to_vector(5)),
                    von_completed(250)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
