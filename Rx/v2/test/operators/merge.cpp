#include "../test.h"
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-merge.hpp>
#include <rxcpp/operators/rx-observe_on.hpp>

const int static_onnextcalls = 1000000;


SCENARIO("synchronize merge ranges", "[hide][range][synchronize][merge][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("some ranges"){
        WHEN("generating ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::synchronize_event_loop();

            int n = 1;
            auto sectionCount = onnextcalls / 3;
            auto start = clock::now();
            int c = rxs::range(0, sectionCount - 1, 1, so)
                .merge(
                    so,
                    rxs::range(sectionCount, (sectionCount * 2) - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge sync ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}

SCENARIO("observe_on merge ranges", "[hide][range][observe_on][merge][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("some ranges"){
        WHEN("generating ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::observe_on_event_loop();

            int n = 1;
            auto sectionCount = onnextcalls / 3;
            auto start = clock::now();
            int c = rxs::range(0, sectionCount - 1, 1, so)
                .merge(
                    so,
                    rxs::range(sectionCount, (sectionCount * 2) - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge observe_on ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}

SCENARIO("serialize merge ranges", "[hide][range][serialize][merge][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("some ranges"){
        WHEN("generating ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::serialize_event_loop();

            int n = 1;
            auto sectionCount = onnextcalls / 3;
            auto start = clock::now();
            int c = rxs::range(0, sectionCount - 1, 1, so)
                .merge(
                    so,
                    rxs::range(sectionCount, (sectionCount * 2) - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge serial ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}

SCENARIO("merge completes", "[merge][join][operators]"){
    GIVEN("1 hot observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(20, 102),
            on.next(110, 103),
            on.next(120, 104),
            on.next(210, 105),
            on.next(220, 106),
            on.completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(10, 201),
            on.next(20, 202),
            on.next(30, 203),
            on.next(40, 204),
            on.completed(50)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(10, 301),
            on.next(20, 302),
            on.next(30, 303),
            on.next(40, 304),
            on.next(120, 305),
            on.completed(150)
        });

        auto xs = sc.make_hot_observable({
            o_on.next(300, ys1),
            o_on.next(400, ys2),
            o_on.next(500, ys3),
            o_on.completed(600)
        });

        WHEN("each int is merged"){

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains merged ints"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(320, 102),
                    on.next(410, 103),
                    on.next(410, 201),
                    on.next(420, 104),
                    on.next(420, 202),
                    on.next(430, 203),
                    on.next(440, 204),
                    on.next(510, 105),
                    on.next(510, 301),
                    on.next(520, 106),
                    on.next(520, 302),
                    on.next(530, 303),
                    on.next(540, 304),
                    on.next(620, 305),
                    on.completed(650)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 530)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(400, 450)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(500, 650)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic merge completes", "[merge][join][operators]"){
    GIVEN("1 hot observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<rx::observable<int>> o_on;

        auto ys1 = sc.make_cold_observable({
            on.next(10, 101),
            on.next(20, 102),
            on.next(110, 103),
            on.next(120, 104),
            on.next(210, 105),
            on.next(220, 106),
            on.completed(230)
        });

        auto ys2 = sc.make_cold_observable({
            on.next(10, 201),
            on.next(20, 202),
            on.next(30, 203),
            on.next(40, 204),
            on.completed(50)
        });

        auto ys3 = sc.make_cold_observable({
            on.next(10, 301),
            on.next(20, 302),
            on.next(30, 303),
            on.next(40, 304),
            on.next(120, 305),
            on.completed(150)
        });

        WHEN("each int is merged"){

            auto res = w.start(
                [&]() {
                    return ys1
                        .merge(ys2, ys3);
                }
            );

            THEN("the output contains merged ints"){
                auto required = rxu::to_vector({
                    on.next(210, 101),
                    on.next(210, 201),
                    on.next(210, 301),
                    on.next(220, 102),
                    on.next(220, 202),
                    on.next(220, 302),
                    on.next(230, 203),
                    on.next(230, 303),
                    on.next(240, 204),
                    on.next(240, 304),
                    on.next(310, 103),
                    on.next(320, 104),
                    on.next(320, 305),
                    on.next(410, 105),
                    on.next(420, 106),
                    on.completed(430)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 430)
                });
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 350)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
