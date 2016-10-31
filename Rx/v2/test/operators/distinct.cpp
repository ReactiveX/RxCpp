#include "../test.h"
#include <rxcpp/operators/rx-distinct.hpp>

SCENARIO("distinct - never", "[distinct][operators]"){
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
                    return xs
                        | rxo::distinct()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
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

SCENARIO("distinct - empty", "[distinct][operators]"){
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
                    return xs.distinct()
                            // forget type to workaround lambda deduction bug on msvc 2013
                            .as_dynamic();
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

SCENARIO("distinct - return", "[distinct][operators]"){
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
                    return xs.distinct()
                            // forget type to workaround lambda deduction bug on msvc 2013
                            .as_dynamic();
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

SCENARIO("distinct - throw", "[distinct][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("distinct on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct()
                            // forget type to workaround lambda deduction bug on msvc 2013
                            .as_dynamic();
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

SCENARIO("distinct - all changes", "[distinct][operators]"){
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
                    return xs.distinct()
                            // forget type to workaround lambda deduction bug on msvc 2013
                            .as_dynamic();
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

SCENARIO("distinct - all same", "[distinct][operators]"){
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
                    return xs.distinct()
                            // forget type to workaround lambda deduction bug on msvc 2013
                            .as_dynamic();
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

SCENARIO("distinct - some changes", "[distinct][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2), //*
            on.next(215, 3), //*
            on.next(220, 3),
            on.next(225, 2),
            on.next(230, 2),
            on.next(230, 1), //*
            on.next(240, 2),
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                [xs]() {
                    return xs.distinct()
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
                }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2), //*
                    on.next(215, 3), //*
                    on.next(230, 1), //*
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

SCENARIO("distinct - strings", "[distinct][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<std::string> on;

        auto xs = sc.make_hot_observable({
            on.next(150, "A"),
            on.next(210, "B"),
            on.next(220, "B"),
            on.next(230, "B"),
            on.next(240, "B"),
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                    [xs]() {
                        return xs.distinct()
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();
                    }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, "B"),
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

SCENARIO("distinct - system_clock's duration", "[distinct][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        using namespace std::chrono;
        const rxsc::test::messages<system_clock::duration> on;

        auto xs = sc.make_hot_observable({
            on.next(150, system_clock::duration{ 10 }),
            on.next(210, system_clock::duration{ 20 }),
            on.next(220, system_clock::duration{ 20 }),
            on.next(230, system_clock::duration{ 100 }),
            on.next(240, system_clock::duration{ 100 }),
            on.completed(250)
        });

        WHEN("distinct values are taken") {

            auto res = w.start(
                [xs]() {
                return xs.distinct()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
            }
            );

            THEN("the output only contains distinct items sent while subscribed") {
                auto required = rxu::to_vector({
                    on.next(210, system_clock::duration{ 20 }),
                    on.next(230, system_clock::duration{ 100 }),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("distinct - high_resolution_clock's duration", "[distinct][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        using namespace std::chrono;
        const rxsc::test::messages<high_resolution_clock::duration> on;

        auto xs = sc.make_hot_observable({
            on.next(150, high_resolution_clock::duration{ 10 }),
            on.next(210, high_resolution_clock::duration{ 20 }),
            on.next(220, high_resolution_clock::duration{ 20 }),
            on.next(230, high_resolution_clock::duration{ 100 }),
            on.next(240, high_resolution_clock::duration{ 100 }),
            on.completed(250)
        });

        WHEN("distinct values are taken") {

            auto res = w.start(
                [xs]() {
                return xs.distinct()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
            }
            );

            THEN("the output only contains distinct items sent while subscribed") {
                auto required = rxu::to_vector({
                    on.next(210, high_resolution_clock::duration{ 20 }),
                    on.next(230, high_resolution_clock::duration{ 100 }),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("distinct - steady_clock's duration", "[distinct][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        using namespace std::chrono;
        const rxsc::test::messages<steady_clock::duration> on;

        auto xs = sc.make_hot_observable({
            on.next(150, steady_clock::duration{ 10 }),
            on.next(210, steady_clock::duration{ 20 }),
            on.next(220, steady_clock::duration{ 20 }),
            on.next(230, steady_clock::duration{ 100 }),
            on.next(240, steady_clock::duration{ 100 }),
            on.completed(250)
        });

        WHEN("distinct values are taken") {

            auto res = w.start(
                [xs]() {
                return xs.distinct()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
            }
            );

            THEN("the output only contains distinct items sent while subscribed") {
                auto required = rxu::to_vector({
                    on.next(210, steady_clock::duration{ 20 }),
                    on.next(230, steady_clock::duration{ 100 }),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("distinct - system_clock's time_point", "[distinct][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        using namespace std::chrono;
        const rxsc::test::messages<system_clock::time_point> on;

        auto xs = sc.make_hot_observable({
            on.next(150, system_clock::time_point{ system_clock::duration{ 10 } }),
            on.next(210, system_clock::time_point{ system_clock::duration{ 20 } }),
            on.next(220, system_clock::time_point{ system_clock::duration{ 20 } }),
            on.next(230, system_clock::time_point{ system_clock::duration{ 100 } }),
            on.next(240, system_clock::time_point{ system_clock::duration{ 100 } }),
            on.completed(250)
        });

        WHEN("distinct values are taken") {

            auto res = w.start(
                [xs]() {
                return xs.distinct()
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains distinct items sent while subscribed") {
                auto required = rxu::to_vector({
                    on.next(210, system_clock::time_point{ system_clock::duration{ 20 } }),
                    on.next(230, system_clock::time_point{ system_clock::duration{ 100 } }),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("distinct - high_resolution_clock's time_point", "[distinct][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        using namespace std::chrono;
        const rxsc::test::messages<high_resolution_clock::time_point> on;

        auto xs = sc.make_hot_observable({
            on.next(150, high_resolution_clock::time_point{ high_resolution_clock::duration{ 10 } }),
            on.next(210, high_resolution_clock::time_point{ high_resolution_clock::duration{ 20 } }),
            on.next(220, high_resolution_clock::time_point{ high_resolution_clock::duration{ 20 } }),
            on.next(230, high_resolution_clock::time_point{ high_resolution_clock::duration{ 100 } }),
            on.next(240, high_resolution_clock::time_point{ high_resolution_clock::duration{ 100 } }),
            on.completed(250)
        });

        WHEN("distinct values are taken") {

            auto res = w.start(
                [xs]() {
                return xs.distinct()
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains distinct items sent while subscribed") {
                auto required = rxu::to_vector({
                    on.next(210, high_resolution_clock::time_point{ high_resolution_clock::duration{ 20 } }),
                    on.next(230, high_resolution_clock::time_point{ high_resolution_clock::duration{ 100 } }),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("distinct - steady_clock's time_point", "[distinct][operators]") {
    GIVEN("a source") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        using namespace std::chrono;
        const rxsc::test::messages<steady_clock::time_point> on;

        auto xs = sc.make_hot_observable({
            on.next(150, steady_clock::time_point{ steady_clock::duration{ 10 } }),
            on.next(210, steady_clock::time_point{ steady_clock::duration{ 20 } }),
            on.next(220, steady_clock::time_point{ steady_clock::duration{ 20 } }),
            on.next(230, steady_clock::time_point{ steady_clock::duration{ 100 } }),
            on.next(240, steady_clock::time_point{ steady_clock::duration{ 100 } }),
            on.completed(250)
        });

        WHEN("distinct values are taken") {

            auto res = w.start(
                [xs]() {
                return xs.distinct()
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains distinct items sent while subscribed") {
                auto required = rxu::to_vector({
                    on.next(210, steady_clock::time_point{ steady_clock::duration{ 20 } }),
                    on.next(230, steady_clock::time_point{ steady_clock::duration{ 100 } }),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("distinct - enum", "[distinct][operators]"){
    enum Value {
        A,
        B,
        C
    };
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<Value> on;

        auto xs = sc.make_hot_observable({
            on.next(150, Value::A),
            on.next(210, Value::A),
            on.next(220, Value::B),
            on.next(230, Value::B),
            on.next(240, Value::B),
            on.completed(250)
        });

        WHEN("distinct values are taken"){

            auto res = w.start(
                    [xs]() {
                        return xs.distinct()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                    }
            );

            THEN("the output only contains distinct items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, Value::A),
                    on.next(220, Value::B),
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