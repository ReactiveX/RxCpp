#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("buffer count partial window", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("group each int with the next 4 ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer(5)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(250, rxu::to_vector({ 2, 3, 4, 5 })),
                    v_on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count full windows", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("group each int with the next int"){

            auto res = w.start(
                [&]() {
                return xs
                    .buffer(2)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(220, rxu::to_vector({ 2, 3 })),
                    v_on.next(240, rxu::to_vector({ 4, 5 })),
                    v_on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count full and partial windows", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("group each int with the next 2 ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(230, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(250, rxu::to_vector({ 5 })),
                    v_on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count error", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        std::runtime_error ex("buffer on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.error(250, ex)
        });

        WHEN("group each int with the next 4 ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer(5)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.error(250, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count skip less", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("group each int with the next 2 ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer(3, 1)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(230, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(240, rxu::to_vector({ 3, 4, 5 })),
                    v_on.next(250, rxu::to_vector({ 4, 5 })),
                    v_on.next(250, rxu::to_vector({ 5 })),
                    v_on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count skip more", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        WHEN("group each int with the next int skipping the third one"){

            auto res = w.start(
                [&]() {
                return xs
                    .buffer(2, 3)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(220, rxu::to_vector({ 2, 3 })),
                    v_on.next(250, rxu::to_vector({ 5 })),
                    v_on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count basic", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

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
                        .buffer(3, 2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(280, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(350, rxu::to_vector({ 4, 5, 6 })),
                    v_on.next(420, rxu::to_vector({ 6, 7, 8 })),
                    v_on.next(600, rxu::to_vector({ 8, 9 })),
                    v_on.completed(600)
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
        }
    }
}

SCENARIO("buffer count disposed", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

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
                        .buffer(3, 2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                370
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(280, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(350, rxu::to_vector({ 4, 5, 6 })),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 370)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer count error 2", "[buffer][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        std::runtime_error ex("buffer on_error from source");

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
                        .buffer(3, 2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(280, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(350, rxu::to_vector({ 4, 5, 6 })),
                    v_on.next(420, rxu::to_vector({ 6, 7, 8 })),
                    v_on.error(600, ex)
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
        }
    }
}

SCENARIO("buffer with time on intervals", "[buffer_with_time][operators][long][hide]"){
    GIVEN("7 intervals of 2 seconds"){
        WHEN("the period is 2sec and the initial is 5sec"){
            // time:   |-----------------|
            // events:      1 2 3 4 5 6 7
            // buffers: ---
            //             -1-
            //                2-3
            //                   -4-
            //                      5-6
            //                         -7
            using namespace std::chrono;
            typedef steady_clock clock;

            #define TIME milliseconds
            #define UNIT *15

            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now() + TIME(5 UNIT);
            auto period = TIME(2 UNIT);

            rx::observable<>::interval(start, period, so)
                .take(7)
                .buffer_with_time(TIME(3 UNIT), so)
                .subscribe(
                    [](std::vector<long> counter){
                        printf("on_next: ");
                        std::for_each(counter.begin(), counter.end(), [](long c){
                            printf("%ld ", c);
                        });
                        printf("\n");
                    },
                    [](std::exception_ptr e){
                        printf("on_error\n");
                    },
                    [](){
                        printf("on_completed\n");
                    }
                );
        }
    }
}

SCENARIO("buffer with time on intervals, implicit coordination", "[buffer_with_time][operators][long][hide]"){
    GIVEN("7 intervals of 2 seconds"){
        WHEN("the period is 2sec and the initial is 5sec"){
            // time:   |-----------------|
            // events:      1 2 3 4 5 6 7
            // buffers: ---
            //             -1-
            //                2-3
            //                   -4-
            //                      5-6
            //                         -7
            using namespace std::chrono;
            typedef steady_clock clock;

            #define TIME milliseconds
            #define UNIT *15

            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now() + TIME(5 UNIT);
            auto period = TIME(2 UNIT);

            rx::observable<>::interval(start, period, so)
                .take(7)
                .buffer_with_time(TIME(3 UNIT))
                .subscribe(
                    [](std::vector<long> counter){
                        printf("on_next: ");
                        std::for_each(counter.begin(), counter.end(), [](long c){
                            printf("%ld ", c);
                        });
                        printf("\n");
                    },
                    [](std::exception_ptr e){
                        printf("on_error\n");
                    },
                    [](){
                        printf("on_completed\n");
                    }
                );
        }
    }
}

SCENARIO("buffer with time on overlapping intervals", "[buffer_with_time][operators][long][hide]"){
    GIVEN("5 intervals of 2 seconds"){
        WHEN("the period is 2sec and the initial is 5sec"){
            // time:   |-------------|
            // events:      1 2 3 4 5
            // buffers: ----
            //            --1-
            //              1-2-
            //                2-3-
            //                  3-4-
            //                    4-5
            //                      5
            using namespace std::chrono;
            typedef steady_clock clock;

            #define TIME milliseconds
            #define UNIT *15

            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now() + TIME(5 UNIT);
            auto period = TIME(2 UNIT);

            rx::observable<>::interval(start, period, so)
                .take(5)
                .buffer_with_time(TIME(4 UNIT), TIME(2 UNIT), so)
                .subscribe(
                    [](std::vector<long> counter){
                        printf("on_next: ");
                        std::for_each(counter.begin(), counter.end(), [](long c){
                            printf("%ld ", c);
                        });
                        printf("\n");
                    },
                    [](std::exception_ptr e){
                        printf("on_error\n");
                    },
                    [](){
                        printf("on_completed\n");
                    }
                );
        }
    }
}

SCENARIO("buffer with time on overlapping intervals, implicit coordination", "[buffer_with_time][operators][long][hide]"){
    GIVEN("5 intervals of 2 seconds"){
        WHEN("the period is 2sec and the initial is 5sec"){
            // time:   |-------------|
            // events:      1 2 3 4 5
            // buffers: ----
            //            --1-
            //              1-2-
            //                2-3-
            //                  3-4-
            //                    4-5
            //                      5
            using namespace std::chrono;
            typedef steady_clock clock;

            #define TIME milliseconds
            #define UNIT *15

            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now() + TIME(5 UNIT);
            auto period = TIME(2 UNIT);

            rx::observable<>::interval(start, period, so)
                .take(5)
                .buffer_with_time(TIME(4 UNIT), TIME(2 UNIT))
                .subscribe(
                    [](std::vector<long> counter){
                        printf("on_next: ");
                        std::for_each(counter.begin(), counter.end(), [](long c){
                            printf("%ld ", c);
                        });
                        printf("\n");
                    },
                    [](std::exception_ptr e){
                        printf("on_error\n");
                    },
                    [](){
                        printf("on_completed\n");
                    }
                );
        }
    }
}

SCENARIO("buffer with time on intervals, error", "[buffer_with_time][operators][long][hide]"){
    GIVEN("5 intervals of 2 seconds"){
        WHEN("the period is 2sec and the initial is 5sec"){
            // time:   |-------------|
            // events:      1 2 3 4 5
            // buffers: ----
            //            --1-
            //              1-2-
            //                2-3-
            //                  3-4-
            //                    4-5
            //                      5
            using namespace std::chrono;
            typedef steady_clock clock;

            #define TIME milliseconds
            #define UNIT *15

            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now() + TIME(5 UNIT);
            auto period = TIME(2 UNIT);

            std::runtime_error ex("buffer_with_time on_error from source");

            auto ys1 = rx::observable<>::interval(start, period, so).take(5);
            auto ys2 = rx::observable<>::error<long, std::runtime_error>(std::runtime_error("buffer_with_time on_error from source"), so);
            ys1.concat(so, ys2)
                .buffer_with_time(TIME(4 UNIT), TIME(2 UNIT), so)
                .subscribe(
                    [](std::vector<long> counter){
                        printf("on_next: ");
                        std::for_each(counter.begin(), counter.end(), [](long c){
                            printf("%ld ", c);
                        });
                        printf("\n");
                    },
                    [](std::exception_ptr e){
                        printf("on_error\n");
                    },
                    [](){
                        printf("on_completed\n");
                    }
                );
        }
    }
}

SCENARIO("buffer with time, overlapping intervals", "[buffer_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

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
        WHEN("group ints on intersecting intervals"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer_with_time(milliseconds(100), milliseconds(70), so)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(300, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(370, rxu::to_vector({ 4, 5, 6 })),
                    v_on.next(440, rxu::to_vector({ 6, 7, 8 })),
                    v_on.next(510, rxu::to_vector({ 8, 9 })),
                    v_on.next(580, std::vector<int>()),
                    v_on.next(600, std::vector<int>()),
                    v_on.completed(600)
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
        }
    }
}

SCENARIO("buffer with time, intervals with skips", "[buffer_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

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
        WHEN("group ints on intervals with skips"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer_with_time(milliseconds(70), milliseconds(100), so)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(270, rxu::to_vector({ 2, 3 })),
                    v_on.next(370, rxu::to_vector({ 5, 6 })),
                    v_on.next(470, rxu::to_vector({ 8, 9 })),
                    v_on.next(570, std::vector<int>()),
                    v_on.completed(600)
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
        }
    }
}

SCENARIO("buffer with time, error", "[buffer_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

        std::runtime_error ex("buffer_with_time on_error from source");

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
        WHEN("group ints on intersecting intervals"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer_with_time(milliseconds(100), milliseconds(70), so)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(300, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(370, rxu::to_vector({ 4, 5, 6 })),
                    v_on.next(440, rxu::to_vector({ 6, 7, 8 })),
                    v_on.next(510, rxu::to_vector({ 8, 9 })),
                    v_on.next(580, std::vector<int>()),
                    v_on.error(600, ex)
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
        }
    }
}

SCENARIO("buffer with time, disposed", "[buffer_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

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
        WHEN("group ints on intersecting intervals"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer_with_time(milliseconds(100), milliseconds(70), so)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                370
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(300, rxu::to_vector({ 2, 3, 4 })),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 370)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("buffer with time, same", "[buffer_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::vector<int>> v_on;

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
        WHEN("group ints on intervals"){
            using namespace std::chrono;

            auto res = w.start(
                [&]() {
                    return xs
                        .buffer_with_time(milliseconds(100), so)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    v_on.next(300, rxu::to_vector({ 2, 3, 4 })),
                    v_on.next(400, rxu::to_vector({ 5, 6, 7 })),
                    v_on.next(500, rxu::to_vector({ 8, 9 })),
                    v_on.next(600, std::vector<int>()),
                    v_on.completed(600)
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
        }
    }
}
