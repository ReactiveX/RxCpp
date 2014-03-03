
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
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        long invoked = 0;

        m::recorded_type messages[] = {
            m::on_next(180, 1),
            m::on_next(210, 2),
            m::on_next(240, 3),
            m::on_next(290, 4),
            m::on_next(350, 5),
            m::on_completed(400),
            m::on_next(410, -1),
            m::on_completed(420),
            m::on_error(430, std::runtime_error("error on unsubscribed stream"))
        };
        auto xs = sc->make_hot_observable(messages);

        WHEN("mapped to ints that are one larger"){

            auto res = sc->start<int>(
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
                m::recorded_type items[] = {
                    m::on_next(210, 3),
                    m::on_next(240, 4),
                    m::on_next(290, 5),
                    m::on_next(350, 6),
                    m::on_completed(400)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rxn::subscription items[] = {
                    m::subscribe(200, 400)
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
