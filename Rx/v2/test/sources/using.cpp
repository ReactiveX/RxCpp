#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("using, cold observable", "[using][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [](){
                                return rxu::to_vector({1, 2, 3, 4, 5});
                            },
                            [&](std::vector<int> values){
                                auto msg = std::vector<rxsc::test::messages<int>::recorded_type>();
                                int time = 10;
                                std::for_each(values.begin(), values.end(), [&](int &v){
                                    msg.push_back(on.on_next(time, v));
                                    time += 10;
                                });
                                msg.push_back(on.on_completed(time));
                                xs.reset(sc.make_cold_observable(msg));
                                return xs.get();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.on_next(210, 1),
                    on.on_next(220, 2),
                    on.on_next(230, 3),
                    on.on_next(240, 4),
                    on.on_next(250, 5),
                    on.on_completed(260)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 260)
                });
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("using, hot observable", "[using][sources]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [](){
                                return rxu::to_vector({ 1, 2, 3, 4, 5 });
                            },
                            [&](std::vector<int> values){
                                auto msg = std::vector<rxsc::test::messages<int>::recorded_type>();
                                int time = 210;
                                std::for_each(values.begin(), values.end(), [&](int &v){
                                    msg.push_back(on.on_next(time, v));
                                    time += 10;
                                });
                                msg.push_back(on.on_completed(time));
                                xs.reset(sc.make_hot_observable(msg));
                                return xs.get();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.on_next(210, 1),
                    on.on_next(220, 2),
                    on.on_next(230, 3),
                    on.on_next(240, 4),
                    on.on_next(250, 5),
                    on.on_completed(260)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 260)
                });
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("using, complete", "[using][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [&](){
                                ++resource_factory_invoked;
                                return sc.clock();
                            },
                            [&](long value){
                                ++observable_factory_invoked;
                                xs.reset(sc.make_cold_observable(rxu::to_vector({
                                    on.on_next(100, value),
                                    on.on_completed(200)
                                })));
                                return xs.get();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("Resource factory is used once"){
                REQUIRE(1 == resource_factory_invoked);
            }

            THEN("Observable factory is used once"){
                REQUIRE(1 == observable_factory_invoked);
            }

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.on_next(300, 200),
                    on.on_completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("using, error", "[using][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("using on_error from source");

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [&](){
                                ++resource_factory_invoked;
                                return sc.clock();
                            },
                            [&](long value){
                                ++observable_factory_invoked;
                                xs.reset(sc.make_cold_observable(rxu::to_vector({
                                    on.on_next(100, value),
                                    on.on_error(200, ex)
                                })));
                                return xs.get();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("Resource factory is used once"){
                REQUIRE(1 == resource_factory_invoked);
            }

            THEN("Observable factory is used once"){
                REQUIRE(1 == observable_factory_invoked);
            }

            THEN("the output stops on error"){
                auto required = rxu::to_vector({
                    on.on_next(300, 200),
                    on.on_error(400, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("using, dispose", "[using][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [&](){
                                ++resource_factory_invoked;
                                return sc.clock();
                            },
                            [&](long value){
                                ++observable_factory_invoked;
                                xs.reset(sc.make_cold_observable(rxu::to_vector({
                                    on.on_next(100, value),
                                    on.on_next(1000, value + 1)
                                })));
                                return xs.get();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("Resource factory is used once"){
                REQUIRE(1 == resource_factory_invoked);
            }

            THEN("Observable factory is used once"){
                REQUIRE(1 == observable_factory_invoked);
            }

            THEN("the output contains resulting ints"){
                auto required = rxu::to_vector({
                    on.on_next(300, 200)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("using, throw resource selector", "[using][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("using on_error from source");

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [&](){
                                ++resource_factory_invoked;
                                throw ex;
                                return 0;
                            },
                            [&](long value){
                                ++observable_factory_invoked;
                                return rx::observable<>::never<int>();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                    }
            );

            THEN("Resource factory is used once"){
                REQUIRE(1 == resource_factory_invoked);
            }

            THEN("Observable factory is not used"){
                REQUIRE(0 == observable_factory_invoked);
            }

            THEN("the output stops on error"){
                auto required = rxu::to_vector({
                    on.on_error(200, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("using, throw resource usage", "[using][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("using on_error from source");

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        WHEN("created by using"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        _using(
                            [&](){
                                ++resource_factory_invoked;
                                return sc.clock();
                            },
                            [&](long value){
                                ++observable_factory_invoked;
                                throw ex;
                                return rx::observable<>::never<int>();
                            }
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                    }
            );

            THEN("Resource factory is used once"){
                REQUIRE(1 == resource_factory_invoked);
            }

            THEN("Observable factory is used once"){
                REQUIRE(1 == observable_factory_invoked);
            }

            THEN("the output stops on error"){
                auto required = rxu::to_vector({
                    on.on_error(200, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}

