#include "../test.h"
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-merge.hpp>
#include <rxcpp/operators/rx-window.hpp>
#include <rxcpp/operators/rx-window_time.hpp>
#include <rxcpp/operators/rx-window_time_count.hpp>

SCENARIO("window count, basic", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = w.start(
                [&]() {
                    return xs
                        .window(3, 2)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(280, 4),
                    on.next(280, 4),
                    on.next(320, 5),
                    on.next(350, 6),
                    on.next(350, 6),
                    on.next(380, 7),
                    on.next(420, 8),
                    on.next(420, 8),
                    on.next(470, 9),
                    on.completed(600)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window count, inner timings", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = rxcpp::observable<rxcpp::observable<int>>();
            auto windows = std::vector<rxcpp::observable<int>>();
            auto observers = std::vector<rxt::testable_observer<int>>();

            w.schedule_absolute(
                rxsc::test::created_time,
                [&](const rxsc::schedulable&) {
                    res = xs
                        | rxo::window(3, 2);
                }
            );

            w.schedule_absolute(
                rxsc::test::subscribed_time,
                [&](const rxsc::schedulable&) {
                    res.subscribe(
                        // on_next
                        [&](rx::observable<int> window) {
                            auto result = w.make_subscriber<int>();
                            windows.push_back(window);
                            observers.push_back(result.get_observer());
                            window.subscribe(result);
                        }
                    );
                }
            );

            w.start();

            THEN("the output contains 5 windows"){
                REQUIRE(5 == observers.size());
            }

            THEN("the 1st output window contains ints"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(280, 4),
                    on.completed(280)
                });
                auto actual = observers[0].messages();
                REQUIRE(required == actual);
            }

            THEN("the 2nd output window contains ints"){
                auto required = rxu::to_vector({
                    on.next(280, 4),
                    on.next(320, 5),
                    on.next(350, 6),
                    on.completed(350)
                });
                auto actual = observers[1].messages();
                REQUIRE(required == actual);
            }

            THEN("the 3rd output window contains ints"){
                auto required = rxu::to_vector({
                    on.next(350, 6),
                    on.next(380, 7),
                    on.next(420, 8),
                    on.completed(420)
                });
                auto actual = observers[2].messages();
                REQUIRE(required == actual);
            }

            THEN("the 4th output window contains ints"){
                auto required = rxu::to_vector({
                    on.next(420, 8),
                    on.next(470, 9),
                    on.completed(600)
                });
                auto actual = observers[3].messages();
                REQUIRE(required == actual);
            }

            THEN("the 5th output window only contains complete message"){
                auto required = rxu::to_vector({
                    on.completed(600)
                });
                auto actual = observers[4].messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window count, dispose", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = w.start(
                [&]() {
                    return xs
                        .window(3, 2)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                370
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(280, 4),
                    on.next(280, 4),
                    on.next(320, 5),
                    on.next(350, 6),
                    on.next(350, 6)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 370)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window count, error", "[window][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("window on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.error(600, ex)
        });

        WHEN("group each int with the next 2 ints"){
            auto res = w.start(
                [&]() {
                    return xs
                        .window(3, 2)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(280, 4),
                    on.next(280, 4),
                    on.next(320, 5),
                    on.next(350, 6),
                    on.next(350, 6),
                    on.next(380, 7),
                    on.next(420, 8),
                    on.next(420, 8),
                    on.next(470, 9),
                    on.error(600, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, basic", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(270, 4),
            on.next(320, 5),
            on.next(360, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(460, 9),
            on.next(470, 10),
            on.completed(490)
        });

        WHEN("group ints by 100 time units"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::window_with_time(milliseconds(100), milliseconds(50), so)
                        | rxo::merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(271, 4),
                    on.next(271, 4),
                    on.next(321, 5),
                    on.next(321, 5),
                    on.next(361, 6),
                    on.next(361, 6),
                    on.next(391, 7),
                    on.next(391, 7),
                    on.next(411, 8),
                    on.next(411, 8),
                    on.next(461, 9),
                    on.next(461, 9),
                    on.next(471, 10),
                    on.next(471, 10),
                    on.completed(491)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 490)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, basic same", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(270, 4),
            on.next(320, 5),
            on.next(360, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(460, 9),
            on.next(470, 10),
            on.completed(490)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time(milliseconds(100), so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(271, 4),
                    on.next(321, 5),
                    on.next(361, 6),
                    on.next(391, 7),
                    on.next(411, 8),
                    on.next(461, 9),
                    on.next(471, 10),
                    on.completed(491)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 490)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, basic 1", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time(milliseconds(100), milliseconds(70), so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(351, 6),
                    on.next(381, 7),
                    on.next(421, 8),
                    on.next(421, 8),
                    on.next(471, 9),
                    on.completed(601)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, basic 2", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time(milliseconds(70), milliseconds(100), so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(421, 8),
                    on.next(471, 9),
                    on.completed(601)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, error", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("window_with_time on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.error(600, ex)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time(milliseconds(100), milliseconds(70), so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(351, 6),
                    on.next(381, 7),
                    on.next(421, 8),
                    on.next(421, 8),
                    on.next(471, 9),
                    on.error(601, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, disposed", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time(milliseconds(100), milliseconds(70), so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                370
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(351, 6),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 371)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time, basic same 1", "[window_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(380, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time(milliseconds(70), so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(381, 7),
                    on.next(421, 8),
                    on.next(471, 9),
                    on.completed(601)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time or count, basic", "[window_with_time_or_count][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(205, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(370, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::window_with_time_or_count(milliseconds(70), 3, so)
                        | rxo::merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(206, 1),
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(371, 7),
                    on.next(421, 8),
                    on.next(471, 9),
                    on.completed(601)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time or count, error", "[window_with_time_or_count][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        std::runtime_error ex("window_with_time on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(205, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(370, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.error(600, ex)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time_or_count(milliseconds(70), 3, so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(206, 1),
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(371, 7),
                    on.next(421, 8),
                    on.next(471, 9),
                    on.error(601, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time or count, disposed", "[window_with_time_or_count][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(205, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4),
            on.next(320, 5),
            on.next(350, 6),
            on.next(370, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.completed(600)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time_or_count(milliseconds(70), 3, so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                372
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(206, 1),
                    on.next(211, 2),
                    on.next(241, 3),
                    on.next(281, 4),
                    on.next(321, 5),
                    on.next(351, 6),
                    on.next(371, 7)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 373)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time or count, only time triggered", "[window_with_time_or_count][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(205, 1),
            on.next(305, 2),
            on.next(505, 3),
            on.next(605, 4),
            on.next(610, 5),
            on.completed(850)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time_or_count(milliseconds(100), 3, so)
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(206, 1),
                    on.next(306, 2),
                    on.next(506, 3),
                    on.next(606, 4),
                    on.next(611, 5),
                    on.completed(851)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 850)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window with time or count, only count triggered", "[window_with_time_or_count][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto xs = sc.make_hot_observable({
            on.next(205, 1),
            on.next(305, 2),
            on.next(505, 3),
            on.next(605, 4),
            on.next(610, 5),
            on.completed(850)
        });

        WHEN("group each int with the next 2 ints"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_with_time_or_count(milliseconds(370), 2, so)
                        .map([](rx::observable<int> w){
                            return w.count();
                        })
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged groups of ints"){
                auto required = rxu::to_vector({
                    on.next(306, 2),
                    on.next(606, 2),
                    on.next(851, 1),
                    on.completed(851)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 850)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
