#include "../test.h"
#include "rxcpp/operators/rx-sequence_equal.hpp"

SCENARIO("sequence_equal - source never emits", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(200, 2),
            on.next(300, 3),
            on.next(400, 4),
            on.next(500, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            | rxo::sequence_equal(ys)
                            | rxo::as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<bool>::recorded_type>();
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

SCENARIO("sequence_equal - other source never emits", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<bool>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - both sources never emit any items", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 0),
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<bool>::recorded_type>();
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

SCENARIO("sequence_equal - both sources emit the same sequence of items", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains true"){
                auto required = rxu::to_vector({
                    o_on.next(610, true),
                    o_on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - first source emits less items than the second one", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(610, false),
                    o_on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - second source emits less items than the first one", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(610, false),
                    o_on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - sources emit different sequence of items", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 9), //
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(330, false),
                    o_on.completed(330)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 330)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - sources emit items in a different order", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 4),
            on.next(440, 3),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(330, false),
                    o_on.completed(330)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 330)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - source observable is empty", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.completed(250)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(330, false),
                    o_on.completed(330)
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

SCENARIO("sequence_equal - other observable is empty", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.completed(400)
        });

        auto ys = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(310, false),
                    o_on.completed(310)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 310)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - both observables are empty", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.completed(400)
        });

        auto ys = sc.make_hot_observable({
            on.completed(250)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains false"){
                auto required = rxu::to_vector({
                    o_on.next(400, true),
                    o_on.completed(400)
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

SCENARIO("sequence_equal - source observable emits an error", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        std::runtime_error ex("sequence_equal error");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains an error"){
                auto required = rxu::to_vector({
                    o_on.error(250, ex)
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

SCENARIO("sequence_equal - other observable emits an error", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        std::runtime_error ex("sequence_equal error");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.completed(400)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains an error"){
                auto required = rxu::to_vector({
                    o_on.error(250, ex)
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

SCENARIO("sequence_equal - both observables emit errors", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        std::runtime_error ex("sequence_equal error1");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.error(250, ex)
        });

        auto ys = sc.make_hot_observable({
            on.error(300, ex)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys)
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains an error"){
                auto required = rxu::to_vector({
                    o_on.error(250, ex)
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

SCENARIO("sequence_equal - both sources emit the same sequence of items, custom comparing function", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys, [](int x, int y) { return x == y; })
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains true"){
                auto required = rxu::to_vector({
                    o_on.next(610, true),
                    o_on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - both sources emit the same sequence of items, custom coordinator", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys, rxcpp::identity_one_worker(rxsc::make_current_thread()))
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains true"){
                auto required = rxu::to_vector({
                    o_on.next(610, true),
                    o_on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sequence_equal - both sources emit the same sequence of items, custom comparing function and coordinator", "[sequence_equal][operators]"){
    GIVEN("two sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<bool> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(310, 3),
            on.next(410, 4),
            on.next(510, 5),
            on.completed(610)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 2),
            on.next(330, 3),
            on.next(440, 4),
            on.next(550, 5),
            on.completed(600)
        });

        WHEN("two observables are checked for equality"){

            auto res = w.start(
                [xs, ys]() {
                    return xs
                            .sequence_equal(ys, [](int x, int y) { return x == y; }, rxcpp::identity_one_worker(rxsc::make_current_thread()))
                            .as_dynamic(); // forget type to workaround lambda deduction bug on msvc 2013
                }
            );

            THEN("the output contains true"){
                auto required = rxu::to_vector({
                    o_on.next(610, true),
                    o_on.completed(610)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 610)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
