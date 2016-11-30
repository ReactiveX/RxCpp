#include "../test.h"
#include <rxcpp/operators/rx-finally.hpp>

SCENARIO("finally - never", "[finally][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("finally action is set"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        | rxo::finally([&invoked]() {
                             ++invoked;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("finally called once"){
                REQUIRE(1 == invoked);
            }

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

SCENARIO("finally - empty", "[finally][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("finally action is set"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        .finally([&invoked]() {
                            ++invoked;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("finally called once"){
                REQUIRE(1 == invoked);
            }

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

SCENARIO("finally - items emitted", "[finally][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;
        
        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(300)
        });

        WHEN("finally action is set"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        .finally([&invoked]() {
                            ++invoked;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("finally called once"){
                REQUIRE(1 == invoked);
            }
            
            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.completed(300)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("finally - throw", "[finally][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        std::runtime_error ex("finally on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("finally action is set"){

            auto res = w.start(
                [xs, &invoked]() {
                    return xs
                        .finally([&invoked]() {
                            ++invoked;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("finally called once"){
                REQUIRE(1 == invoked);
            }

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
