#include "../test.h"
#include <rxcpp/operators/rx-switch_if_empty.hpp>

SCENARIO("switch_if_empty should not switch if the source is not empty", "[switch_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys = sc.make_cold_observable({
            on.next(10, 2),
            on.completed(230)
        });

        auto xs = sc.make_hot_observable({
            on.next(210, 1),
            on.completed(250)
        });

        WHEN("started"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                        | rxo::switch_if_empty(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output only contains an item from the source") {
                auto required = rxu::to_vector({
                    on.next(210, 1),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was no subscription/unsubscription to ys"){
                auto required = std::vector<rxn::subscription>();
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_if_empty should switch if the source is empty", "[switch_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys = sc.make_cold_observable({
            on.next(10, 2),
            on.completed(20)
        });

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("started"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                        .switch_if_empty(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains an item from the backup source") {
                auto required = rxu::to_vector({
                    on.next(260, 2),
                    on.completed(270)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys"){
                auto required = rxu::to_vector({
                    on.subscribe(250, 270)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_if_empty - never", "[switch_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys = sc.make_cold_observable({
            on.next(10, 2),
            on.completed(20)
        });

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("started"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                        .switch_if_empty(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was no subscription/unsubscription to ys"){
                auto required = std::vector<rxn::subscription>();
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_if_empty - source throws", "[switch_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("switch_if_empty on_error from source");

        auto ys = sc.make_cold_observable({
            on.next(10, 2),
            on.completed(20)
        });

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("started"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                        .switch_if_empty(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains an error from the source"){
                auto required = rxu::to_vector({
                    on.error(250, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was no subscription/unsubscription to ys"){
                auto required = std::vector<rxn::subscription>();
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_if_empty - backup source throws", "[switch_if_empty][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("switch_if_empty on_error from backup source");

        auto ys = sc.make_cold_observable({
            on.error(10, ex)
        });

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("started"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                        .switch_if_empty(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains an error from the backup source"){
                auto required = rxu::to_vector({
                    on.error(260, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys"){
                auto required = rxu::to_vector({
                    on.subscribe(250, 260)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
