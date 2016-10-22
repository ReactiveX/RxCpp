#include "../test.h"
#include <rxcpp/operators/rx-debounce.hpp>

using namespace std::chrono;

SCENARIO("debounce - never", "[debounce][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("values are debounceed"){

            auto res = w.start(
                [so, xs]() {
                    return xs | rxo::debounce(so, milliseconds(10));
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1001)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("debounce - empty", "[debounce][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("values are debounceed"){

            auto res = w.start(
                [so, xs]() {
                    return xs.debounce(milliseconds(10), so);
                }
            );

            THEN("the output only contains complete message"){
                auto required = rxu::to_vector({
                    on.completed(251)
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

SCENARIO("debounce - no overlap", "[debounce][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(300)
        });

        WHEN("values are debounceed"){

            auto res = w.start(
                [so, xs]() {
                    return xs.debounce(milliseconds(10), so);
                }
            );

            THEN("the output only contains debounced items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(221, 2),
                    on.next(251, 3),
                    on.completed(301)
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

SCENARIO("debounce - overlap", "[debounce][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.next(225, 3),
            on.next(235, 4),
            on.next(245, 5),
            on.next(255, 6),
            on.next(265, 7),
            on.completed(300)
        });

        WHEN("values are debounceed"){

            auto res = w.start(
                [so, xs]() {
                    return xs.debounce(milliseconds(30), so);
                }
            );

            THEN("the output only contains debounced items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(296, 7),
                    on.completed(301)
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

SCENARIO("debounce - throw", "[debounce][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("debounce on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("values are debounceed"){

            auto res = w.start(
                [so, xs]() {
                    return xs.debounce(milliseconds(10), so);
                }
            );

            THEN("the output only contains only error"){
                auto required = rxu::to_vector({
                    on.error(251, ex)
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
