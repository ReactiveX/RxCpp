#include "../test.h"
#include <rxcpp/operators/rx-on_error_resume_next.hpp>

SCENARIO("switch_on_error stops on completion", "[switch_on_error][on_error_resume_next][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.next(350, 5),
            on.completed(400),
            on.next(410, -1),
            on.completed(420),
            on.error(430, std::runtime_error("error on unsubscribed stream"))
        });

        auto ys = sc.make_cold_observable({
            on.next(10, -1),
            on.completed(20),
        });

        WHEN("passed through unchanged"){

            auto res = w.start(
                [xs, ys, &invoked]() {
                    return xs
                        .switch_on_error([ys, &invoked](std::exception_ptr) {
                            invoked++;
                            return ys;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(290, 4),
                    on.next(350, 5),
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one xs subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was no ys subscription"){
                auto required = std::vector<rxcpp::notifications::subscription>();
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("switch_on_error selector was not called"){
                REQUIRE(0 == invoked);
            }
        }
    }
}

SCENARIO("on_error_resume_next stops on completion", "[on_error_resume_next][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.next(350, 5),
            on.completed(400),
            on.next(410, -1),
            on.completed(420),
            on.error(430, std::runtime_error("error on unsubscribed stream"))
        });

        auto ys = sc.make_cold_observable({
            on.next(10, -1),
            on.completed(20),
        });

        WHEN("passed through unchanged"){

            auto res = w.start(
                [xs, ys, &invoked]() {
                    return xs
                        .on_error_resume_next([ys, &invoked](std::exception_ptr) {
                            invoked++;
                            return ys;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(290, 4),
                    on.next(350, 5),
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one xs subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was no ys subscription"){
                auto required = std::vector<rxcpp::notifications::subscription>();
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("on_error_resume_next selector was not called"){
                REQUIRE(0 == invoked);
            }
        }
    }
}

SCENARIO("on_error_resume_next stops on error", "[on_error_resume_next][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        std::runtime_error ex("on_error_resume_next on_error from source");
        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(180, 1),
            on.next(210, 2),
            on.next(240, 3),
            on.next(290, 4),
            on.error(300, ex),
            on.next(350, 5),
            on.completed(400),
            on.next(410, -1),
            on.completed(420),
            on.error(430, std::runtime_error("error on unsubscribed stream"))
        });

        auto ys = sc.make_cold_observable({
            on.next(10, -1),
            on.completed(20),
        });

        WHEN("are resumed after an error"){

            auto res = w.start(
                [xs, ys, &invoked]() {
                    return xs
                        .on_error_resume_next([ys, &invoked](std::exception_ptr) {
                            invoked++;
                            return ys;
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(240, 3),
                    on.next(290, 4),
                    on.next(310, -1),
                    on.completed(320)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one xs subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 300)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one ys subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 320)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("on_error_resume_next selector was called once"){
                REQUIRE(1 == invoked);
            }
        }
    }
}
