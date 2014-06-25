#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("take 2", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        WHEN("2 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(220, 3),
                    on.on_completed(220)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, complete after", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10),
            on.on_completed(690)
        });

        WHEN("20 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(20)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_next(280, 1),
                    on.on_next(300, -1),
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11),
                    on.on_next(410, 15),
                    on.on_next(415, 16),
                    on.on_next(460, 72),
                    on.on_next(510, 76),
                    on.on_next(560, 32),
                    on.on_next(570, -100),
                    on.on_next(580, -3),
                    on.on_next(590, 5),
                    on.on_next(630, 10),
                    on.on_completed(690)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 690)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, complete same", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10),
            on.on_completed(690)
        });

        WHEN("17 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(17)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_next(280, 1),
                    on.on_next(300, -1),
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11),
                    on.on_next(410, 15),
                    on.on_next(415, 16),
                    on.on_next(460, 72),
                    on.on_next(510, 76),
                    on.on_next(560, 32),
                    on.on_next(570, -100),
                    on.on_next(580, -3),
                    on.on_next(590, 5),
                    on.on_next(630, 10),
                    on.on_completed(630)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 630)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, complete before", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10),
            on.on_completed(690)
        });

        WHEN("10 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(10)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_next(280, 1),
                    on.on_next(300, -1),
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11),
                    on.on_next(410, 15),
                    on.on_next(415, 16),
                    on.on_completed(415)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 415)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, error after", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take on_error from source");

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10),
            on.on_error(690, ex)
        });

        WHEN("20 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(20)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_next(280, 1),
                    on.on_next(300, -1),
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11),
                    on.on_next(410, 15),
                    on.on_next(415, 16),
                    on.on_next(460, 72),
                    on.on_next(510, 76),
                    on.on_next(560, 32),
                    on.on_next(570, -100),
                    on.on_next(580, -3),
                    on.on_next(590, 5),
                    on.on_next(630, 10),
                    on.on_error(690, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 690)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, error same", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10),
            on.on_error(690, std::runtime_error("error in unsubscribed stream"))
        });

        WHEN("17 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(17)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_next(280, 1),
                    on.on_next(300, -1),
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11),
                    on.on_next(410, 15),
                    on.on_next(415, 16),
                    on.on_next(460, 72),
                    on.on_next(510, 76),
                    on.on_next(560, 32),
                    on.on_next(570, -100),
                    on.on_next(580, -3),
                    on.on_next(590, 5),
                    on.on_next(630, 10),
                    on.on_completed(630)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 630)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, error before", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10),
            on.on_error(690, std::runtime_error("error in unsubscribed stream"))
        });

        WHEN("3 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_completed(270)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 270)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, dispose before", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10)
        });

        WHEN("3 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                250
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13)
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

        }
    }
}

SCENARIO("take, dispose after", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(70, 6),
            on.on_next(150, 4),
            on.on_next(210, 9),
            on.on_next(230, 13),
            on.on_next(270, 7),
            on.on_next(280, 1),
            on.on_next(300, -1),
            on.on_next(310, 3),
            on.on_next(340, 8),
            on.on_next(370, 11),
            on.on_next(410, 15),
            on.on_next(415, 16),
            on.on_next(460, 72),
            on.on_next(510, 76),
            on.on_next(560, 32),
            on.on_next(570, -100),
            on.on_next(580, -3),
            on.on_next(590, 5),
            on.on_next(630, 10)
        });

        WHEN("3 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 9),
                    on.on_next(230, 13),
                    on.on_next(270, 7),
                    on.on_completed(270)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 270)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}



SCENARIO("take_until trigger on_next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        auto ys = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(225, 99),
            on.on_completed(230)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                        .take_until(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(220, 3),
                    on.on_completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt some data next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(225, 99),
            on.on_completed(230)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(220, 3),
                    on.on_completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt some data error", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_until on_error from source");

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(225, ex)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(220, 3),
                    on.on_error(225, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt some data empty", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_completed(225)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(220, 3),
                    on.on_next(230, 4),
                    on.on_next(240, 5),
                    on.on_completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt some data never", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_next(220, 3),
            on.on_next(230, 4),
            on.on_next(240, 5),
            on.on_completed(250)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(210, 2),
                    on.on_next(220, 3),
                    on.on_next(230, 4),
                    on.on_next(240, 5),
                    on.on_completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt never next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(225, 2), //!
            on.on_completed(250)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt never error", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_until on_error from source");

        auto l = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(225, ex)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_error(225, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt never empty", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_completed(225)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000 /* can't dispose prematurely, could be in flight to dispatch OnError */)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt never never", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt before first produced", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(230, 2),
            on.on_completed(240)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2), //!
            on.on_completed(220)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt before first produced, remain silent and proper unsubscribed", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        bool sourceNotDisposed = false;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(215, std::runtime_error("error in unsubscribed stream")), // should not come
            on.on_completed(240)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2), //!
            on.on_completed(220)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r, &sourceNotDisposed]() {
                    return l
                        .map([&sourceNotDisposed](int v){sourceNotDisposed = true; return v;})
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("signal disposed"){
                auto required = false;
                auto actual = sourceNotDisposed;
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt after last produced, proper unsubscribe signal", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        bool signalNotDisposed = false;

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(230, 2),
            on.on_completed(240)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(250, 2),
            on.on_completed(260)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r, &signalNotDisposed]() {
                    return l
                        .take_until(r
                            .map([&signalNotDisposed](int v){signalNotDisposed = true; return v;}))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(230, 2),
                    on.on_completed(240)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("signal disposed"){
                auto required = false;
                auto actual = signalNotDisposed;
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, error some", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_until on_error from source");

        auto l = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(225, ex)
        });

        auto r = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(240, 2)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_error(225, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
