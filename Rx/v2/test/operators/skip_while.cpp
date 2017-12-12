#include "../test.h"
#include <rxcpp/operators/rx-skip_while.hpp>

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

SCENARIO("skip_while not equal to 4", "[skip_while][operators]"){
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
                        .skip_while(not_equal_to(4))
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(230, 4),
                    on.next(240, 5),
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

        }
    }
}

SCENARIO("skip_while, complete after", "[skip_while][operators]"){
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

        WHEN("none are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip_while(not_equal_to(0))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains no items sent while subscribed"){
                auto required = rxu::to_vector({
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

SCENARIO("skip_while, complete before", "[skip_while][operators]"){
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

        WHEN("7 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip_while(not_equal_to(72))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
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

SCENARIO("skip_while, error after", "[skip_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("skip_while on_error from source");

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

        WHEN("no values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip_while(not_equal_to(0))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed and the error"){
                auto required = rxu::to_vector({
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

SCENARIO("skip_while, error before", "[skip_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

		std::runtime_error ex("skip_while on_error from source");

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
			on.error(500, ex),
            on.next(510, 76),
            on.next(560, 32),
            on.next(570, -100),
            on.next(580, -3),
            on.next(590, 5),
            on.next(630, 10)
        });

        WHEN("only one value is taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip_while(not_equal_to(72))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
					on.next(460, 72),
					on.error(500, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 500)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("skip_while, dispose before", "[skip_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
            on.next(150, 100),
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
                        .skip_while(not_equal_to(100))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                250
            );

            THEN("the output only contains items sent while subscribed"){
				std::vector<rxcpp::notifications::recorded<std::shared_ptr<rxcpp::notifications::detail::notification_base<int> > > > required;
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

SCENARIO("skip_while, dispose after", "[skip_while][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 6),
			on.next(150, 4), //this is skipped due to delayed subscription
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

        WHEN("5 values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs
                        .skip_while(not_equal_to(1))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
    				on.next(280, 1),
					on.next(300, -1),
					on.next(310, 3),
					on.next(340, 8),
					on.next(370, 11),
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
