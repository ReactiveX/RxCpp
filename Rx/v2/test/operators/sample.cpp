#include "../test.h"
#include "rxcpp/operators/rx-sample_time.hpp"

SCENARIO("sample with time, error", "[sample_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("sample_with_time on_error from source");

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
                        | rxo::sample_with_time(milliseconds(100), so)
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    on.next(301, 4),
                    on.next(401, 7),
                    on.next(501, 9),
                    on.error(601, ex)
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

SCENARIO("sample with time, disposed", "[sample_with_time][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(100, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(280, 4), //
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
                        .sample_with_time(milliseconds(100), so)
                        .as_dynamic();
                },
                370
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    on.next(301, 4),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 371)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("sample with time, same", "[sample_with_time][operators]"){
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
                        .sample_with_time(milliseconds(100), so)
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    on.next(301, 4),
                    on.next(401, 7),
                    on.next(501, 9),
                    on.completed(601)
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
