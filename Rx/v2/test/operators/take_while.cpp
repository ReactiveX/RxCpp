#include "../test.h"
#include <rxcpp/operators/rx-take_while.hpp>

namespace {
    class not_equal_to {
        int value;
    public:
        not_equal_to(int value) : value(value) { }
        bool operator()(int i) const {
            return i != value;
        }
    };
}

SCENARIO("take_while not equal to 4", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("values before 4 are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(4))
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
                    on.completed(230)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_while, complete after", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10),
            on.completed(690)
        });

        WHEN("all are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(0))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13),
                    on.next(270, 7),
                    on.next(280, 1),
                    on.next(300, -1),
                    on.next(310, 3),
                    on.next(340, 8),
                    on.next(370, 11),
                    on.next(410, 15),
                    on.next(415, 16),
                    on.next(460, 72),
                    on.next(510, 76),
                    on.next(560, 32),
                    on.next(570, -100),
                    on.next(580, -3),
                    on.next(590, 5),
                    on.next(630, 10),
                    on.completed(690)
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

SCENARIO("take_while, complete before", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10),
            on.completed(690)
        });

        WHEN("10 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(72))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13),
                    on.next(270, 7),
                    on.next(280, 1),
                    on.next(300, -1),
                    on.next(310, 3),
                    on.next(340, 8),
                    on.next(370, 11),
                    on.next(410, 15),
                    on.next(415, 16),
                    on.completed(460)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 460)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_while, error after", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_while on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10),
            on.error(690, ex)
        });

        WHEN("all values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(0))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed and the error"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13),
                    on.next(270, 7),
                    on.next(280, 1),
                    on.next(300, -1),
                    on.next(310, 3),
                    on.next(340, 8),
                    on.next(370, 11),
                    on.next(410, 15),
                    on.next(415, 16),
                    on.next(460, 72),
                    on.next(510, 76),
                    on.next(560, 32),
                    on.next(570, -100),
                    on.next(580, -3),
                    on.next(590, 5),
                    on.next(630, 10),
                    on.error(690, ex)
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

SCENARIO("take_while, error same", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10),
            on.error(690, std::runtime_error("error in unsubscribed stream"))
        });

        WHEN("all but one values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(10))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13),
                    on.next(270, 7),
                    on.next(280, 1),
                    on.next(300, -1),
                    on.next(310, 3),
                    on.next(340, 8),
                    on.next(370, 11),
                    on.next(410, 15),
                    on.next(415, 16),
                    on.next(460, 72),
                    on.next(510, 76),
                    on.next(560, 32),
                    on.next(570, -100),
                    on.next(580, -3),
                    on.next(590, 5),
                    on.completed(630)
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

SCENARIO("take_while, error before", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10),
            on.error(690, std::runtime_error("error in unsubscribed stream"))
        });

        WHEN("3 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(1))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13),
                    on.next(270, 7),
                    on.completed(280)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 280)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_while, dispose before", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10)
        });

        WHEN("3 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(100))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                250
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13)
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

SCENARIO("take_while, dispose after", "[take_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 4),
            on.next(210, 9),
            on.next(230, 13),
            on.next(270, 7),
            on.next(280, 1),
            on.next(300, -1),
            on.next(310, 3),
            on.next(340, 8),
            on.next(370, 11),
            on.next(410, 15),
            on.next(415, 16),
            on.next(460, 72),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10)
        });

        WHEN("3 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .take_while(not_equal_to(1))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 9),
                    on.next(230, 13),
                    on.next(270, 7),
                    on.completed(280)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 280)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

