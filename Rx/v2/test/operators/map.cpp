
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

SCENARIO("map stops on completion", "[map][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(180, 1),
            on_next(210, 2),
            on_next(240, 3),
            on_next(290, 4),
            on_next(350, 5),
            on_completed(400),
            on_next(410, -1),
            on_completed(420),
            on_error(430, std::runtime_error("error on unsubscribed stream"))
        };
        auto xs = sc.make_hot_observable(messages);

        WHEN("mapped to ints that are one larger"){

            auto res = w.start<int>(
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
                record items[] = {
                    on_next(210, 3),
                    on_next(240, 4),
                    on_next(290, 5),
                    on_next(350, 6),
                    on_completed(400)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 400)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("map was called until completed"){
                REQUIRE(4 == invoked);
            }
        }
    }
}
