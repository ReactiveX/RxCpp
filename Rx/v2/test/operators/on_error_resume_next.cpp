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
                        .switch_on_error([ys, &invoked](rxu::error_ptr) {
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
                        .on_error_resume_next([ys, &invoked](rxu::error_ptr) {
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
                        .on_error_resume_next([ys, &invoked](rxu::error_ptr) {
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


SCENARIO("on_error_resume_next doesn't provide copies", "[on_error_resume_next][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = verifier.get_observable().on_error_resume_next([](const auto&)
                                                                           {
                                                                               return rxcpp::observable<>::empty<copy_verifier>();
                                                                           });
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to final lambda
                REQUIRE(verifier.get_copy_count() == 1);
                REQUIRE(verifier.get_move_count() == 0);
            }
        }
    }
}


SCENARIO("on_error_resume_next doesn't provide copies for move", "[on_error_resume_next][operators][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto obs = verifier.get_observable_for_move().on_error_resume_next([](const auto&)
                                                                           {
                                                                               return rxcpp::observable<>::empty<copy_verifier>();
                                                                           });
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                // 1 move to final lambda
                REQUIRE(verifier.get_move_count() == 1);
            }
        }
    }
}
