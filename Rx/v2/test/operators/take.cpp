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

SCENARIO("take 2", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("2 values are taken"){

            auto res = sc.start<int>(
                [xs]() {
                    return xs
                        .take(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_completed(220)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 220)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}


SCENARIO("take_until trigger on_next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto xs = sc.make_hot_observable(xmessages);

        record ymessages[] = {
            on_next(150, 1),
            on_next(225, 99),
            on_completed(230)
        };
        auto ys = sc.make_hot_observable(ymessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = sc.start<int>(
                [xs, ys]() {
                    return xs
                        .take_until(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_completed(225)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
