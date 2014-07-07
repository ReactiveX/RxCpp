#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;
namespace rxn=rx::notifications;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("switch_on_next - some changes", "[switch_on_next][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_cold_observable({
            on.on_next(10, 101),
            on.on_next(20, 102),
            on.on_next(110, 103),
            on.on_next(120, 104),
            on.on_next(210, 105),
            on.on_next(220, 106),
            on.on_completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.on_next(10, 201),
            on.on_next(20, 202),
            on.on_next(30, 203),
            on.on_next(40, 204),
            on.on_completed(50)
        });

        auto ys3 = sc.make_cold_observable({
            on.on_next(10, 301),
            on.on_next(20, 302),
            on.on_next(30, 303),
            on.on_next(40, 304),
            on.on_completed(150)
        });

        auto xs = sc.make_hot_observable({
            o_on.on_next(300, ys1),
            o_on.on_next(400, ys2),
            o_on.on_next(500, ys3),
            o_on.on_completed(600)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(310, 101),
                    on.on_next(320, 102),
                    on.on_next(410, 201),
                    on.on_next(420, 202),
                    on.on_next(430, 203),
                    on.on_next(440, 204),
                    on.on_next(510, 301),
                    on.on_next(520, 302),
                    on.on_next(530, 303),
                    on.on_next(540, 304),
                    on.on_completed(650)
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
            on.on_next(10, 101),
            on.on_next(20, 102),
            on.on_next(110, 103),
            on.on_next(120, 104),
            on.on_next(210, 105),
            on.on_next(220, 106),
            on.on_completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.on_next(10, 201),
            on.on_next(20, 202),
            on.on_next(30, 203),
            on.on_next(40, 204),
            on.on_error(50, ex)
        });

        auto ys3 = sc.make_cold_observable({
            on.on_next(10, 301),
            on.on_next(20, 302),
            on.on_next(30, 303),
            on.on_next(40, 304),
            on.on_completed(150)
        });

        auto xs = sc.make_hot_observable({
            o_on.on_next(300, ys1),
            o_on.on_next(400, ys2),
            o_on.on_next(500, ys3),
            o_on.on_completed(600)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(310, 101),
                    on.on_next(320, 102),
                    on.on_next(410, 201),
                    on.on_next(420, 202),
                    on.on_next(430, 203),
                    on.on_next(440, 204),
                    on.on_error(450, ex)
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
            on.on_next(10, 101),
            on.on_next(20, 102),
            on.on_next(110, 103),
            on.on_next(120, 104),
            on.on_next(210, 105),
            on.on_next(220, 106),
            on.on_completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.on_next(10, 201),
            on.on_next(20, 202),
            on.on_next(30, 203),
            on.on_next(40, 204),
            on.on_completed(50)
        });

        auto xs = sc.make_hot_observable({
            o_on.on_next(300, ys1),
            o_on.on_next(400, ys2),
            o_on.on_error(500, ex)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(310, 101),
                    on.on_next(320, 102),
                    on.on_next(410, 201),
                    on.on_next(420, 202),
                    on.on_next(430, 203),
                    on.on_next(440, 204),
                    on.on_error(500, ex)
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
            o_on.on_completed(500)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_completed(500)
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
            on.on_next(10, 101),
            on.on_next(20, 102),
            on.on_next(110, 103),
            on.on_next(120, 104),
            on.on_next(210, 105),
            on.on_next(220, 106),
            on.on_completed(230)
        });

        auto xs = sc.make_hot_observable({
            o_on.on_next(300, ys1),
            o_on.on_completed(540)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.switch_on_next();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(310, 101),
                    on.on_next(320, 102),
                    on.on_next(410, 103),
                    on.on_next(420, 104),
                    on.on_next(510, 105),
                    on.on_next(520, 106),
                    on.on_completed(540)
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
