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

SCENARIO("flat_map completes", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;

        m::recorded_type int_messages[] = {
            m::on_next(100, 4),
            m::on_next(200, 2),
            m::on_next(300, 3),
            m::on_next(400, 1),
            m::on_completed(500)
        };
        auto xs = sc->make_cold_observable(int_messages);

        ms::recorded_type string_messages[] = {
            ms::on_next(50, "foo"),
            ms::on_next(100, "bar"),
            ms::on_next(150, "baz"),
            ms::on_next(200, "qux"),
            ms::on_completed(250)
        };
        auto ys = sc->make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = sc->start<std::string>(
                [&]() {
                    return xs
                        .flat_map(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                ms::recorded_type items[] = {
                    ms::on_next(350, "foo"),
                    ms::on_next(400, "bar"),
                    ms::on_next(450, "baz"),
                    ms::on_next(450, "foo"),
                    ms::on_next(500, "qux"),
                    ms::on_next(500, "bar"),
                    ms::on_next(550, "baz"),
                    ms::on_next(550, "foo"),
                    ms::on_next(600, "qux"),
                    ms::on_next(600, "bar"),
                    ms::on_next(650, "baz"),
                    ms::on_next(650, "foo"),
                    ms::on_next(700, "bar"),
                    ms::on_next(700, "qux"),
                    ms::on_next(750, "baz"),
                    ms::on_next(800, "qux"),
                    ms::on_completed(850)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                rxn::subscription items[] = {
                    m::subscribe(200, 700)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                rxn::subscription items[] = {
                    ms::subscribe(300, 550),
                    ms::subscribe(400, 650),
                    ms::subscribe(500, 750),
                    ms::subscribe(600, 850)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}


SCENARIO("flat_map source never ends", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;

        m::recorded_type int_messages[] = {
            m::on_next(100, 4),
            m::on_next(200, 2),
            m::on_next(300, 3),
            m::on_next(400, 1),
            m::on_next(500, 5),
            m::on_next(700, 0)
        };
        auto xs = sc->make_cold_observable(int_messages);

        ms::recorded_type string_messages[] = {
            ms::on_next(50, "foo"),
            ms::on_next(100, "bar"),
            ms::on_next(150, "baz"),
            ms::on_next(200, "qux"),
            ms::on_completed(250)
        };
        auto ys = sc->make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = sc->start<std::string>(
                [&]() {
                    return xs
                        .flat_map([&](int){return ys;}, [](int, std::string s){return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                ms::recorded_type items[] = {
                    ms::on_next(350, "foo"),
                    ms::on_next(400, "bar"),
                    ms::on_next(450, "baz"),
                    ms::on_next(450, "foo"),
                    ms::on_next(500, "bar"),
                    ms::on_next(500, "qux"),
                    ms::on_next(550, "baz"),
                    ms::on_next(550, "foo"),
                    ms::on_next(600, "bar"),
                    ms::on_next(600, "qux"),
                    ms::on_next(650, "baz"),
                    ms::on_next(650, "foo"),
                    ms::on_next(700, "bar"),
                    ms::on_next(700, "qux"),
                    ms::on_next(750, "baz"),
                    ms::on_next(750, "foo"),
                    ms::on_next(800, "bar"),
                    ms::on_next(800, "qux"),
                    ms::on_next(850, "baz"),
                    ms::on_next(900, "qux"),
                    ms::on_next(950, "foo")
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                rxn::subscription items[] = {
                    m::subscribe(200, 1000)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                rxn::subscription items[] = {
                    ms::subscribe(300, 550),
                    ms::subscribe(400, 650),
                    ms::subscribe(500, 750),
                    ms::subscribe(600, 850),
                    ms::subscribe(700, 950),
                    ms::subscribe(900, 1000)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("flat_map inner error", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;

        m::recorded_type int_messages[] = {
            m::on_next(100, 4),
            m::on_next(200, 2),
            m::on_next(300, 3),
            m::on_next(400, 1),
            m::on_completed(500)
        };
        auto xs = sc->make_cold_observable(int_messages);

        std::runtime_error ex("filter on_error from inner source");

        ms::recorded_type string_messages[] = {
            ms::on_next(55, "foo"),
            ms::on_next(104, "bar"),
            ms::on_next(153, "baz"),
            ms::on_next(202, "qux"),
            ms::on_error(301, ex)
        };
        auto ys = sc->make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = sc->start<std::string>(
                [&]() {
                    return xs
                        .flat_map([&](int){return ys;}, [](int, std::string s){return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                ms::recorded_type items[] = {
                    ms::on_next(355, "foo"),
                    ms::on_next(404, "bar"),
                    ms::on_next(453, "baz"),
                    ms::on_next(455, "foo"),
                    ms::on_next(502, "qux"),
                    ms::on_next(504, "bar"),
                    ms::on_next(553, "baz"),
                    ms::on_next(555, "foo"),
                    ms::on_error(601, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                rxn::subscription items[] = {
                    m::subscribe(200, 601)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                rxn::subscription items[] = {
                    ms::subscribe(300, 601),
                    ms::subscribe(400, 601),
                    ms::subscribe(500, 601),
                    ms::subscribe(600, 601)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
