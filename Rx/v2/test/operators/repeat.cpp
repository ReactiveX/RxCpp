#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("repeat, basic test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_cold_observable({
            on.on_next(100, 1),
            on.on_next(150, 2),
            on.on_next(200, 3),
            on.on_completed(250)
        });

        WHEN("infinite repeat is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains 3 sets of ints"){
                auto required = rxu::to_vector({
                    on.on_next(300, 1),
                    on.on_next(350, 2),
                    on.on_next(400, 3),
                    on.on_next(550, 1),
                    on.on_next(600, 2),
                    on.on_next(650, 3),
                    on.on_next(800, 1),
                    on.on_next(850, 2),
                    on.on_next(900, 3)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 4 subscriptions and 4 unsubscriptions to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 450),
                    on.subscribe(450, 700),
                    on.subscribe(700, 950),
                    on.subscribe(950, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("repeat, infinite observable test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints that never completes."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_cold_observable({
            on.on_next(100, 1),
            on.on_next(150, 2),
            on.on_next(200, 3)
        });

        WHEN("infinite repeat is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains a set of ints"){
                auto required = rxu::to_vector({
                    on.on_next(300, 1),
                    on.on_next(350, 2),
                    on.on_next(400, 3)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription and 1 unsubscription to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("repeat, error test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints followed by an error."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("repeat on_error from source");

        auto xs = sc.make_cold_observable({
            on.on_next(100, 1),
            on.on_next(150, 2),
            on.on_next(200, 3),
            on.on_error(250, ex)
        });

        WHEN("infinite repeat is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains a set of ints and an error"){
                auto required = rxu::to_vector({
                    on.on_next(300, 1),
                    on.on_next(350, 2),
                    on.on_next(400, 3),
                    on.on_error(450, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription and 1 unsubscription to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 450)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("countable repeat, basic test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_cold_observable({
            on.on_next(5, 1),
            on.on_next(10, 2),
            on.on_next(15, 3),
            on.on_completed(20)
        });

        WHEN("repeat of 3 iterations is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains 3 sets of ints"){
                auto required = rxu::to_vector({
                    on.on_next(205, 1),
                    on.on_next(210, 2),
                    on.on_next(215, 3),
                    on.on_next(225, 1),
                    on.on_next(230, 2),
                    on.on_next(235, 3),
                    on.on_next(245, 1),
                    on.on_next(250, 2),
                    on.on_next(255, 3),
                    on.on_completed(260)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 3 subscriptions and 3 unsubscriptions to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220),
                    on.subscribe(220, 240),
                    on.subscribe(240, 260)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("countable repeat, dispose test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_cold_observable({
            on.on_next(5, 1),
            on.on_next(10, 2),
            on.on_next(15, 3),
            on.on_completed(20)
        });

        WHEN("repeat of 3 iterations is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                231
            );

            THEN("the output contains less than 2 full sets of ints"){
                auto required = rxu::to_vector({
                    on.on_next(205, 1),
                    on.on_next(210, 2),
                    on.on_next(215, 3),
                    on.on_next(225, 1),
                    on.on_next(230, 2),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 2 subscriptions and 2 unsubscriptions to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220),
                    on.subscribe(220, 231)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("countable repeat, infinite observable test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints that never completes."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_cold_observable({
            on.on_next(100, 1),
            on.on_next(150, 2),
            on.on_next(200, 3)
        });

        WHEN("infinite repeat is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains a set of ints"){
                auto required = rxu::to_vector({
                    on.on_next(300, 1),
                    on.on_next(350, 2),
                    on.on_next(400, 3)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription and 1 unsubscription to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("countable repeat, error test", "[repeat][operators]"){
    GIVEN("cold observable of 3 ints followed by an error."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("repeat on_error from source");

        auto xs = sc.make_cold_observable({
            on.on_next(100, 1),
            on.on_next(150, 2),
            on.on_next(200, 3),
            on.on_error(250, ex)
        });

        WHEN("infinite repeat is launched"){

            auto res = w.start(
                [&]() {
                    return xs
                        .repeat(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains a set of ints and an error"){
                auto required = rxu::to_vector({
                    on.on_next(300, 1),
                    on.on_next(350, 2),
                    on.on_next(400, 3),
                    on.on_error(450, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription and 1 unsubscription to the ints"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 450)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
