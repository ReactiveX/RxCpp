#include "../test.h"

SCENARIO("distinct_until_changed - never", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
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
        }
    }
}

SCENARIO("distinct_until_changed - empty", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains complete message"){
                auto required = rxu::to_vector({
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

SCENARIO("distinct_until_changed - return", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
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

SCENARIO("distinct_until_changed - throw", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("distinct_until_changed on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains only error"){
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

        }
    }
}

SCENARIO("distinct_until_changed - all changes", "[distinct_until_changed][operators]"){
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

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
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

SCENARIO("distinct_until_changed - all same", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 2),
            on.next(230, 2),
            on.next(240, 2),
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
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

SCENARIO("distinct_until_changed - some changes", "[distinct_until_changed][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2), //*
            on.next(215, 3), //*
            on.next(220, 3),
            on.next(225, 2), //*
            on.next(230, 2),
            on.next(230, 1), //*
            on.next(240, 2), //*
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct_until_changed();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2), //*
                    on.next(215, 3), //*
                    on.next(225, 2), //*
                    on.next(230, 1), //*
                    on.next(240, 2), //*
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
