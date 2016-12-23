#include "../test.h"
#include "rxcpp/operators/rx-amb.hpp"

SCENARIO("amb never 3", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2)
        });

        auto ys3 = sc.make_hot_observable({
            on.next(120, 3)
        });

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.completed(200)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 1000)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 1000)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 1000)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb never empty", "[amb][join][operators]"){
    GIVEN("1 cold observable with 2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2),
            on.completed(400)
        });

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.completed(150)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
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

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 350)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb completes", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

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

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.completed(100)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(410, 102),
                    on.next(510, 103),
                    on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 610)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb winner throws", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

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

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.completed(100)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(410, 102),
                    on.next(510, 103),
                    on.error(610, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 610)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb loser throws", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

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

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.completed(100)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(410, 102),
                    on.next(510, 103),
                    on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 610)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb throws before selection", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("amb on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(110, 1),
            on.completed(200)
        });

        auto ys2 = sc.make_cold_observable({
            on.error(50, ex)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(130, 3),
            on.completed(300)
        });

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.completed(100)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only an error"){
                auto required = rxu::to_vector({
                    on.error(350, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 350)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 350)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 350)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb throws before selection and emission end", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("amb on_error from source");

        auto ys1 = sc.make_cold_observable({
            on.next(110, 1),
            on.completed(200)
        });

        auto ys2 = sc.make_cold_observable({
            on.error(50, ex)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(130, 3),
            on.completed(300)
        });

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.completed(500)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only an error"){
                auto required = rxu::to_vector({
                    on.error(350, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 350)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 350)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 350)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 350)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb loser comes when winner has already emitted", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

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

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(200, ys3),
            o_on.completed(200)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(410, 102),
                    on.next(510, 103),
                    on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 610)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were no subscriptions to the ys3"){
                auto required = std::vector<rxn::subscription>();
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb empty list", "[amb][join][operators]"){
    GIVEN("1 empty cold observable of observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_cold_observable({
            o_on.completed(200)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only comlpete message"){
                auto required = rxu::to_vector({
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb source throws before selection", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("amb on_error from source");

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

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.error(100, ex)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only an error"){
                auto required = rxu::to_vector({
                    on.error(300, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 300)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 300)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 300)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb source throws after selection", "[amb][join][operators]"){
    GIVEN("1 cold observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("amb on_error from source");

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

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.next(100, ys3),
            o_on.error(300, ex)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints from the first observable"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(410, 102),
                    on.error(500, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 500)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 500)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 310)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("amb never empty, custom coordination", "[amb][join][operators]"){
    GIVEN("1 cold observable with 2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_hot_observable({
            on.next(100, 1)
        });

        auto ys2 = sc.make_hot_observable({
            on.next(110, 2),
            on.completed(400)
        });

        auto xs = sc.make_cold_observable({
            o_on.next(100, ys1),
            o_on.next(100, ys2),
            o_on.completed(150)
        });

        WHEN("the first observable is selected to produce ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .amb(so)
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
