#include "../test.h"
#include "rxcpp/operators/rx-timeout.hpp"

using namespace std::chrono;

SCENARIO("should timeout if the source never emits any items", "[timeout][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        rxcpp::timeout_error ex("timeout has occurred");

        auto xs = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("timeout is set"){

            auto res = w.start(
                [so, xs]() {
                    return xs
                        | rxo::timeout(milliseconds(10), so);
                }
            );

            THEN("the error notification message is captured"){
                auto required = rxu::to_vector({
                    on.error(211, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 212)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("should not timeout if completed before the specified timeout duration", "[timeout][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(250)
        });

        WHEN("timeout is set"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timeout(so, milliseconds(100));
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

SCENARIO("should not timeout if all items are emitted within the specified timeout duration", "[timeout][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.completed(250)
        });

        WHEN("timeout is set"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timeout(milliseconds(40), so);
                }
            );

            THEN("the output contains the emitted items while subscribed"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
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

SCENARIO("should timeout if there are no emitted items within the timeout duration", "[timeout][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        rxcpp::timeout_error ex("timeout has occurred");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(240, 3),
            // -- no emissions
            on.completed(300)
        });

        WHEN("timeout is set"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timeout(milliseconds(40), so);
                }
            );

            THEN("an error notification message is captured"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(241, 3),
                    on.error(281, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 282)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("should not timeout if there is an error", "[timeout][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.error(250, ex)
        });

        WHEN("timeout is set"){

            auto res = w.start(
                [so, xs]() {
                    return xs.timeout(milliseconds(100), so);
                }
            );

            THEN("the output contains only an error message"){
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
