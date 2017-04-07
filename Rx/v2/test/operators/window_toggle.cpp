#include "../test.h"
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-merge.hpp>
#include <rxcpp/operators/rx-window_toggle.hpp>

SCENARIO("window toggle, basic", "[window_toggle][operators]"){
    GIVEN("1 hot observable of ints and hot observable of opens."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::string> o_on;

        auto xs = sc.make_hot_observable({
            on.next(90, 1),
            on.next(180, 2),
            on.next(250, 3),
            on.next(260, 4),
            on.next(310, 5),
            on.next(340, 6),
            on.next(410, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.next(550, 10),
            on.completed(590)
        });

        auto ys = sc.make_hot_observable({
            on.next(255, 50),
            on.next(330, 100),
            on.next(350, 50),
            on.next(400, 90),
            on.completed(900)
        });

        WHEN("ints are split into windows"){
            using namespace std::chrono;

            int wi = 0;

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::window_toggle(ys, [&](int y){
                            return rx::observable<>::timer(milliseconds(y), so);
                        }, so)
                        | rxo::map([wi](rxcpp::observable<int> w) mutable {
                            auto ti = wi++;
                            return w
                                | rxo::map([ti](int x){return std::to_string(ti) + " " + std::to_string(x);})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                | rxo::as_dynamic();
                        })
                        | rxo::merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains ints assigned to windows"){
                auto required = rxu::to_vector({
                    o_on.next(261, "0 4"),
                    o_on.next(341, "1 6"),
                    o_on.next(411, "1 7"),
                    o_on.next(411, "3 7"),
                    o_on.next(421, "1 8"),
                    o_on.next(421, "3 8"),
                    o_on.next(471, "3 9"),
                    o_on.completed(591)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 590)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window toggle, basic same", "[window_toggle][operators]"){
    GIVEN("1 hot observable of ints and hot observable of opens."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::string> o_on;

        auto xs = sc.make_hot_observable({
            on.next(90, 1),
            on.next(180, 2),
            on.next(250, 3),
            on.next(260, 4),
            on.next(310, 5),
            on.next(340, 6),
            on.next(410, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.next(550, 10),
            on.completed(590)
        });

        auto ys = sc.make_hot_observable({
            on.next(255, 50),
            on.next(330, 100),
            on.next(350, 50),
            on.next(400, 90),
            on.completed(900)
        });

        WHEN("ints are split into windows"){
            using namespace std::chrono;

            int wi = 0;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_toggle(ys, [&](int){
                            return ys;
                        }, so)
                        .map([wi](rxcpp::observable<int> w) mutable {
                            auto ti = wi++;
                            return w
                                .map([ti](int x){return std::to_string(ti) + " " + std::to_string(x);})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();
                        })
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints assigned to windows"){
                auto required = rxu::to_vector({
                    o_on.next(261, "0 4"),
                    o_on.next(311, "0 5"),
                    o_on.next(341, "1 6"),
                    o_on.next(411, "3 7"),
                    o_on.next(421, "3 8"),
                    o_on.next(471, "3 9"),
                    o_on.next(551, "3 10"),
                    o_on.completed(591)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 590)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window toggle, error", "[window_toggle][operators]"){
    GIVEN("1 hot observable of ints and hot observable of opens."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::string> o_on;

        std::runtime_error ex("window_toggle on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(90, 1),
            on.next(180, 2),
            on.next(250, 3),
            on.next(260, 4),
            on.next(310, 5),
            on.next(340, 6),
            on.next(410, 7),
            on.error(420, ex),
            on.next(470, 9),
            on.next(550, 10),
            on.completed(590)
        });

        auto ys = sc.make_hot_observable({
            on.next(255, 50),
            on.next(330, 100),
            on.next(350, 50),
            on.next(400, 90),
            on.completed(900)
        });

        WHEN("ints are split into windows"){
            using namespace std::chrono;

            int wi = 0;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_toggle(ys, [&](int){
                            return ys;
                        }, so)
                        .map([wi](rxcpp::observable<int> w) mutable {
                            auto ti = wi++;
                            return w
                                .map([ti](int x){return std::to_string(ti) + " " + std::to_string(x);})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();
                        })
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains ints assigned to windows"){
                auto required = rxu::to_vector({
                    o_on.next(261, "0 4"),
                    o_on.next(311, "0 5"),
                    o_on.next(341, "1 6"),
                    o_on.next(411, "3 7"),
                    o_on.error(421, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 420)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("window toggle, disposed", "[window_toggle][operators]"){
    GIVEN("1 hot observable of ints and hot observable of opens."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        const rxsc::test::messages<std::string> o_on;

        auto xs = sc.make_hot_observable({
            on.next(90, 1),
            on.next(180, 2),
            on.next(250, 3),
            on.next(260, 4),
            on.next(310, 5),
            on.next(340, 6),
            on.next(410, 7),
            on.next(420, 8),
            on.next(470, 9),
            on.next(550, 10),
            on.completed(590)
        });

        auto ys = sc.make_hot_observable({
            on.next(255, 50),
            on.next(330, 100),
            on.next(350, 50),
            on.next(400, 90),
            on.completed(900)
        });

        WHEN("ints are split into windows"){
            using namespace std::chrono;

            int wi = 0;

            auto res = w.start(
                [&]() {
                    return xs
                        .window_toggle(ys, [&](int){
                            return ys;
                        }, so)
                        .map([wi](rxcpp::observable<int> w) mutable {
                            auto ti = wi++;
                            return w
                                .map([ti](int x){return std::to_string(ti) + " " + std::to_string(x);})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();
                        })
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                420
            );

            THEN("the output contains ints assigned to windows"){
                auto required = rxu::to_vector({
                    o_on.next(261, "0 4"),
                    o_on.next(311, "0 5"),
                    o_on.next(341, "1 6"),
                    o_on.next(411, "3 7")
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the observable"){
                auto required = rxu::to_vector({
                    o_on.subscribe(200, 420)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

