#include "rxcpp/rx.hpp"
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
