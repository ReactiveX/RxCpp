#include "../test.h"
#include "rxcpp/operators/rx-amb.hpp"

SCENARIO("variadic amb never 3", "[amb][join][operators]"){
    GIVEN("3 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2)
        });

        auto ys3 = sc.make_hot_observable({
            on.next(120, 3)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        | rxo::amb(ys2, ys3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb never empty", "[amb][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2),
            on.completed(400)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .amb(ys2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb empty never", "[amb][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1),
            on.completed(400)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .amb(ys2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb completes", "[amb][join][operators]"){
    GIVEN("3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(110, 102),
            on.next(210, 103),
            on.completed(310)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(20, 201),
            on.next(120, 202),
            on.next(220, 203),
            on.completed(320)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(30, 301),
            on.next(130, 302),
            on.next(230, 303),
            on.completed(330)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .amb(ys2, ys3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(210, 101),
                    on.next(310, 102),
                    on.next(410, 103),
                    on.completed(510)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 510)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb winner&owner throws", "[amb][join][operators]"){
    GIVEN("3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("amb on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(110, 102),
            on.next(210, 103),
            on.error(310, ex)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(20, 201),
            on.next(120, 202),
            on.next(220, 203),
            on.completed(320)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(30, 301),
            on.next(130, 302),
            on.next(230, 303),
            on.completed(330)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .amb(ys2, ys3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(210, 101),
                    on.next(310, 102),
                    on.next(410, 103),
                    on.error(510, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 510)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb winner&non-owner throws", "[amb][join][operators]"){
    GIVEN("3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("amb on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(110, 102),
            on.next(210, 103),
            on.error(310, ex)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(20, 201),
            on.next(120, 202),
            on.next(220, 203),
            on.completed(320)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(30, 301),
            on.next(130, 302),
            on.next(230, 303),
            on.completed(330)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys2
                        .amb(ys1, ys3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(210, 101),
                    on.next(310, 102),
                    on.next(410, 103),
                    on.error(510, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 510)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb loser&non-owner throws", "[amb][join][operators]"){
    GIVEN("3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("amb on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(110, 102),
            on.next(210, 103),
            on.completed(310)
        });

        auto ys2 = sc.make_cold_observable({
            on.error(20, ex)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(30, 301),
            on.next(130, 302),
            on.next(230, 303),
            on.completed(330)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys2
                        .amb(ys1, ys3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(210, 101),
                    on.next(310, 102),
                    on.next(410, 103),
                    on.completed(510)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 510)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb loser&owner throws", "[amb][join][operators]"){
    GIVEN("3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("amb on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(110, 102),
            on.next(210, 103),
            on.completed(310)
        });

        auto ys2 = sc.make_cold_observable({
            on.error(20, ex)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(30, 301),
            on.next(130, 302),
            on.next(230, 303),
            on.completed(330)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .amb(ys2, ys3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(210, 101),
                    on.next(310, 102),
                    on.next(410, 103),
                    on.completed(510)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 510)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic amb never empty, custom coordination", "[amb][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2),
            on.completed(400)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .amb(so, ys2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(401)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}
