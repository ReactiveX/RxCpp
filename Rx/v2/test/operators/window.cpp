#include "rxcpp/rx.hpp"
namespace rx = rxcpp;
namespace rxu = rxcpp::util;
namespace rxs = rxcpp::sources;
namespace rxsc = rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
namespace rxt = rxcpp::test;

#include "catch.hpp"

SCENARIO("window count, basic", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.on_next(100, 1),
            on.on_next(210, 2),
            on.on_next(240, 3),
            on.on_next(280, 4),
            on.on_next(320, 5),
            on.on_next(350, 6),
            on.on_next(380, 7),
            on.on_next(420, 8),
            on.on_next(470, 9),
            on.on_completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = w.start(
                [&]() {
                    return xs
                        .window(3, 2)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(240, 3),
                    on.on_next(280, 4),
                    on.on_next(280, 4),
                    on.on_next(320, 5),
                    on.on_next(350, 6),
                    on.on_next(350, 6),
                    on.on_next(380, 7),
                    on.on_next(420, 8),
                    on.on_next(420, 8),
                    on.on_next(470, 9),
                    on.on_completed(600)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window count, inner timings", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.on_next(100, 1),
            on.on_next(210, 2),
            on.on_next(240, 3),
            on.on_next(280, 4),
            on.on_next(320, 5),
            on.on_next(350, 6),
            on.on_next(380, 7),
            on.on_next(420, 8),
            on.on_next(470, 9),
            on.on_completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = rxcpp::observable<rxcpp::observable<int>>();
            auto windows = std::vector<rxcpp::observable<int>>();
            auto observers = std::vector<rxt::testable_observer<int>>();

            w.schedule_absolute(
                rxsc::test::created_time,
                [&](const rxsc::schedulable& scbl) {
                    res = xs
                        .window(3, 2);
                }
            );

            w.schedule_absolute(
                rxsc::test::subscribed_time,
                [&](const rxsc::schedulable& scbl) {
                    res.subscribe(
                        // on_next
                        [&](rx::observable<int> window) {
                            auto result = w.make_subscriber<int>();
                            windows.push_back(window);
                            observers.push_back(result.get_observer());
                            window.subscribe(result);
                        }
                    );
                }
            );

            w.start();

            THEN("the output contains 5 windows"){
                REQUIRE(5 == observers.size());
            }

            THEN("the 1st output window contains ints"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(240, 3),
                    on.on_next(280, 4),
                    on.on_completed(280)
                });
                auto actual = observers[0].messages();
                REQUIRE(required == actual);
            }

            THEN("the 2nd output window contains ints"){
                auto required = rxu::to_vector({
                    on.on_next(280, 4),
                    on.on_next(320, 5),
                    on.on_next(350, 6),
                    on.on_completed(350)
                });
                auto actual = observers[1].messages();
                REQUIRE(required == actual);
            }

            THEN("the 3rd output window contains ints"){
                auto required = rxu::to_vector({
                    on.on_next(350, 6),
                    on.on_next(380, 7),
                    on.on_next(420, 8),
                    on.on_completed(420)
                });
                auto actual = observers[2].messages();
                REQUIRE(required == actual);
            }

            THEN("the 4th output window contains ints"){
                auto required = rxu::to_vector({
                    on.on_next(420, 8),
                    on.on_next(470, 9),
                    on.on_completed(600)
                });
                auto actual = observers[3].messages();
                REQUIRE(required == actual);
            }

            THEN("the 5th output window only contains complete message"){
                auto required = rxu::to_vector({
                    on.on_completed(600)
                });
                auto actual = observers[4].messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window count, dispose", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.on_next(100, 1),
            on.on_next(210, 2),
            on.on_next(240, 3),
            on.on_next(280, 4),
            on.on_next(320, 5),
            on.on_next(350, 6),
            on.on_next(380, 7),
            on.on_next(420, 8),
            on.on_next(470, 9),
            on.on_completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = w.start(
                [&]() {
                    return xs
                        .window(3, 2)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                370
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(240, 3),
                    on.on_next(280, 4),
                    on.on_next(280, 4),
                    on.on_next(320, 5),
                    on.on_next(350, 6),
                    on.on_next(350, 6)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 370)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window count, error", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("window on_error from source");

        auto xs = sc.make_hot_observable({
            on.on_next(100, 1),
            on.on_next(210, 2),
            on.on_next(240, 3),
            on.on_next(280, 4),
            on.on_next(320, 5),
            on.on_next(350, 6),
            on.on_next(380, 7),
            on.on_next(420, 8),
            on.on_next(470, 9),
            on.on_error(600, ex)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = w.start(
                [&]() {
                    return xs
                        .window(3, 2)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(240, 3),
                    on.on_next(280, 4),
                    on.on_next(280, 4),
                    on.on_next(320, 5),
                    on.on_next(350, 6),
                    on.on_next(350, 6),
                    on.on_next(380, 7),
                    on.on_next(420, 8),
                    on.on_next(420, 8),
                    on.on_next(470, 9),
                    on.on_error(600, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
