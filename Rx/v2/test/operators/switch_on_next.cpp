#include "../test.h"
#include <rxcpp/operators/rx-switch_on_next.hpp>

SCENARIO("switch_on_next - some changes", "[switch_on_next][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(20, 102),
            on.next(110, 103),
            on.next(120, 104),
            on.next(210, 105),
            on.next(220, 106),
            on.completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(10, 201),
            on.next(20, 202),
            on.next(30, 203),
            on.next(40, 204),
            on.completed(50)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(10, 301),
            on.next(20, 302),
            on.next(30, 303),
            on.next(40, 304),
            on.completed(150)
        });

        auto xs = sc.make_hot_observable({
            o_on.next(300, ys1),
            o_on.next(400, ys2),
            o_on.next(500, ys3),
            o_on.completed(600)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        | rxo::switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(320, 102),
                    on.next(410, 201),
                    on.next(420, 202),
                    on.next(430, 203),
                    on.next(440, 204),
                    on.next(510, 301),
                    on.next(520, 302),
                    on.next(530, 303),
                    on.next(540, 304),
                    on.completed(650)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(400, 450)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(500, 650)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_on_next - inner throws", "[switch_on_next][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("switch_on_next on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(20, 102),
            on.next(110, 103),
            on.next(120, 104),
            on.next(210, 105),
            on.next(220, 106),
            on.completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(10, 201),
            on.next(20, 202),
            on.next(30, 203),
            on.next(40, 204),
            on.error(50, ex)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(10, 301),
            on.next(20, 302),
            on.next(30, 303),
            on.next(40, 304),
            on.completed(150)
        });

        auto xs = sc.make_hot_observable({
            o_on.next(300, ys1),
            o_on.next(400, ys2),
            o_on.next(500, ys3),
            o_on.completed(600)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(320, 102),
                    on.next(410, 201),
                    on.next(420, 202),
                    on.next(430, 203),
                    on.next(440, 204),
                    on.error(450, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 450)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(400, 450)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys3"){
                auto required = std::vector<rxn::subscription>();
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_on_next - outer throws", "[switch_on_next][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("switch_on_next on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(20, 102),
            on.next(110, 103),
            on.next(120, 104),
            on.next(210, 105),
            on.next(220, 106),
            on.completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(10, 201),
            on.next(20, 202),
            on.next(30, 203),
            on.next(40, 204),
            on.completed(50)
        });

        auto xs = sc.make_hot_observable({
            o_on.next(300, ys1),
            o_on.next(400, ys2),
            o_on.error(500, ex)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(320, 102),
                    on.next(410, 201),
                    on.next(420, 202),
                    on.next(430, 203),
                    on.next(440, 204),
                    on.error(500, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 500)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(400, 450)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_on_next - no inner", "[switch_on_next][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            o_on.completed(500)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.completed(500)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 500)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("switch_on_next - inner completes", "[switch_on_next][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(20, 102),
            on.next(110, 103),
            on.next(120, 104),
            on.next(210, 105),
            on.next(220, 106),
            on.completed(230)
        });

        auto xs = sc.make_hot_observable({
            o_on.next(300, ys1),
            o_on.completed(540)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(320, 102),
                    on.next(410, 103),
                    on.next(420, 104),
                    on.next(510, 105),
                    on.next(520, 106),
                    on.completed(540)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 540)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 530)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
