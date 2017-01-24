#include "../test.h"
#include <rxcpp/operators/rx-concat.hpp>
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-observe_on.hpp>

const int static_onnextcalls = 1000000;

SCENARIO("synchronize concat ranges", "[hide][range][synchronize][concat][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("some ranges"){
        WHEN("generating ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::synchronize_event_loop();

            int n = 1;
            auto sectionCount = onnextcalls / 3;
            auto start = clock::now();
            auto c = rxs::range(0, sectionCount - 1, 1, so)
                .concat(
                    so,
                    rxs::range(sectionCount, sectionCount * 2 - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "concat sync ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}

SCENARIO("observe_on concat ranges", "[hide][range][observe_on][concat][perf]"){
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
                .concat(
                    so,
                    rxs::range(sectionCount, sectionCount * 2 - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "concat observe_on ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}

SCENARIO("serialize concat ranges", "[hide][range][serialize][concat][perf]"){
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
                .concat(
                    so,
                    rxs::range(sectionCount, sectionCount * 2 - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "concat serial ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}


SCENARIO("concat completes", "[concat][join][operators]"){
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
                        | rxo::concat()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains merged ints"){
                auto required = rxu::to_vector({
                    on.next(310, 101),
                    on.next(320, 102),
                    on.next(410, 103),
                    on.next(420, 104),
                    on.next(510, 105),
                    on.next(520, 106),
                    on.next(540, 201),
                    on.next(550, 202),
                    on.next(560, 203),
                    on.next(570, 204),
                    on.next(590, 301),
                    on.next(600, 302),
                    on.next(610, 303),
                    on.next(620, 304),
                    on.next(700, 305),
                    on.completed(730)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
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
                    on.subscribe(530, 580)
                });
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                auto required = rxu::to_vector({
                    on.subscribe(580, 730)
                });
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
