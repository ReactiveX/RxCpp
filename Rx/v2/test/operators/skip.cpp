#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("skip, complete after", "[skip][operators]"){
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

        WHEN("more values than generated are skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(20)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains only complete message"){
                auto required = rxu::to_vector({
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

SCENARIO("skip, complete same", "[skip][operators]"){
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

        WHEN("exact number of values is skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(17)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains only complete message"){
                auto required = rxu::to_vector({
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

SCENARIO("skip, complete before", "[skip][operators]"){
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

        WHEN("part of values is skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(10)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
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

SCENARIO("skip, complete zero", "[skip][operators]"){
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

        WHEN("no values are skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(0)
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

SCENARIO("skip, error after", "[skip][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("skip on_error from source");

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

        WHEN("more values than generated are skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(20)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains only error message"){
                auto required = rxu::to_vector({
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

SCENARIO("skip, error same", "[skip][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("skip on_error from source");

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

        WHEN("exact number of values is skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(17)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains only error message"){
                auto required = rxu::to_vector({
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

SCENARIO("skip, error before", "[skip][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("skip on_error from source");

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

        WHEN("part of values is skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
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

SCENARIO("skip, dispose before", "[skip][operators]"){
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

        WHEN("all generated values are skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                250
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
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

SCENARIO("skip, dispose after", "[skip][operators]"){
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

        WHEN("some generated values are skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(280, 1),
                    on.on_next(300, -1),
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("skip, consecutive", "[skip][operators]"){
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
            on.on_completed(400)
        });

        WHEN("3+2 values are skipped"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip(3)
                        .skip(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(310, 3),
                    on.on_next(340, 8),
                    on.on_next(370, 11),
                    on.on_completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
