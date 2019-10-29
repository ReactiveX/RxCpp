#include "../test.h"
#include <rxcpp/operators/rx-throttle.hpp>

using namespace std::chrono;

SCENARIO("throttle - never", "[throttle][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("values are throttled"){

            auto res = w.start(
                [so, xs]() {
                    return xs | rxo::throttle(so, milliseconds(10));
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

SCENARIO("throttle - empty", "[throttle][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("values are throttled"){

            auto res = w.start(
                [so, xs]() {
                    return xs.throttle(milliseconds(10), so);
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

SCENARIO("throttle - no overlap", "[throttle][operators]"){
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

        WHEN("values are throtteled"){

            auto res = w.start(
                [so, xs]() {
                    return xs.throttle(milliseconds(10), so);
                }
            );

            THEN("the output only contains throttled items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
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

SCENARIO("throttle - overlap", "[throttle][operators]"){
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
            on.next(247, 5),
            on.next(255, 6),
            on.next(265, 7),
            on.next(500, 8),
            on.completed(600)
        });

        WHEN("values are throttled"){

            auto res = w.start(
                [so, xs]() {
                    return xs.throttle(milliseconds(30), so);
                }
            );

            THEN("the output only contains throttled items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(215, 2),
                    on.next(247, 5),
                    on.next(500, 8),
                    on.completed(601)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("throttle - throw", "[throttle][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("throttle on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("values are throttled"){

            auto res = w.start(
                [so, xs]() {
                    return xs.throttle(milliseconds(10), so);
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
