
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

SCENARIO("defer stops on completion", "[defer][operators]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<long> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<long>> xs;

        WHEN("deferred"){

            auto res = w.start<long>(
                [&]() {
                    return rx::observable<>::defer(
                        [&](){
                            invoked++;
                            record messages[] = {
                                on_next(100, sc.clock()),
                                on_completed(200)
                            };
                            xs.reset(sc.make_cold_observable(messages));
                            return xs.get();
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                record items[] = {
                    on_next(300, 200L),
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
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }

            THEN("defer was called until completed"){
                REQUIRE(1 == invoked);
            }
        }
    }
}
