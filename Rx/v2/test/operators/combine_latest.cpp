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

SCENARIO("combine_latest interleaved with tail", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<rx::observable<int>> mo;
        typedef rxn::subscription life;

        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record messages1[] = {
            on_next(150, 1),
            on_next(215, 2),
            on_next(225, 4),
            on_completed(230)
        };
        auto o1 = sc.make_hot_observable(messages1);

        record messages2[] = {
            on_next(150, 1),
            on_next(220, 3),
            on_next(230, 5),
            on_next(235, 6),
            on_next(240, 7),
            on_completed(250)
        };
        auto o2 = sc.make_hot_observable(messages2);

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start<int>(
                [&]() {
                    return o2
                        .combine_latest([](int v2, int v1){return v2 + v1;}, o1)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                record items[] = {
                    on_next(220, 2 + 3),
                    on_next(225, 4 + 3),
                    on_next(230, 4 + 5),
                    on_next(235, 4 + 6),
                    on_next(240, 4 + 7),
                    on_completed(250)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                life items[] = {
                    subscribe(200, 230)
                };
                auto required = rxu::to_vector(items);
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
