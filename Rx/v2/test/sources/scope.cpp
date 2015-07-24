#include "../test.h"

SCENARIO("scope, cold observable", "[scope][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        typedef rx::resource<std::vector<int>> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&](){
                                return resource(rxu::to_vector({1, 2, 3, 4, 5}));
                            },
                            [&](resource r){
                                auto msg = std::vector<rxsc::test::messages<int>::recorded_type>();
                                int time = 10;
                                auto values = r.get();
                                std::for_each(values.begin(), values.end(), [&](int &v){
                                    msg.push_back(on.next(time, v));
                                    time += 10;
                                });
                                msg.push_back(on.completed(time));
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
                    on.next(210, 1),
                    on.next(220, 2),
                    on.next(230, 3),
                    on.next(240, 4),
                    on.next(250, 5),
                    on.completed(260)
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

SCENARIO("scope, hot observable", "[scope][sources]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        typedef rx::resource<std::vector<int>> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&](){
                                return resource(rxu::to_vector({1, 2, 3, 4, 5}));
                            },
                            [&](resource r){
                                auto msg = std::vector<rxsc::test::messages<int>::recorded_type>();
                                int time = 210;
                                auto values = r.get();
                                std::for_each(values.begin(), values.end(), [&](int &v){
                                    msg.push_back(on.next(time, v));
                                    time += 10;
                                });
                                msg.push_back(on.completed(time));
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
                    on.next(210, 1),
                    on.next(220, 2),
                    on.next(230, 3),
                    on.next(240, 4),
                    on.next(250, 5),
                    on.completed(260)
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

SCENARIO("scope, complete", "[scope][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        typedef rx::resource<int> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&](){
                                ++resource_factory_invoked;
                                return resource(sc.clock());
                            },
                            [&](resource r){
                                ++observable_factory_invoked;
                                xs.reset(sc.make_cold_observable(rxu::to_vector({
                                    on.next(100, r.get()),
                                    on.completed(200)
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
                    on.next(300, 200),
                    on.completed(400)
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

SCENARIO("scope, error", "[scope][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("scope on_error from source");

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        typedef rx::resource<int> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&](){
                                ++resource_factory_invoked;
                                return resource(sc.clock());
                            },
                            [&](resource r){
                                ++observable_factory_invoked;
                                xs.reset(sc.make_cold_observable(rxu::to_vector({
                                    on.next(100, r.get()),
                                    on.error(200, ex)
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
                    on.next(300, 200),
                    on.error(400, ex)
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

SCENARIO("scope, dispose", "[scope][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<int>> xs;

        typedef rx::resource<int> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&](){
                                ++resource_factory_invoked;
                                return resource(sc.clock());
                            },
                            [&](resource r){
                                ++observable_factory_invoked;
                                xs.reset(sc.make_cold_observable(rxu::to_vector({
                                    on.next(100, r.get()),
                                    on.next(1000, r.get() + 1)
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
                    on.next(300, 200)
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

SCENARIO("scope, throw resource selector", "[scope][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("scope on_error from source");

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        typedef rx::resource<int> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&]() -> resource {
                                ++resource_factory_invoked;
                                throw ex;
                                //return resource(sc.clock());
                            },
                            [&](resource){
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
                    on.error(200, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("scope, throw resource usage", "[scope][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("scope on_error from source");

        int resource_factory_invoked = 0;
        int observable_factory_invoked = 0;

        typedef rx::resource<int> resource;

        WHEN("created by scope"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::
                        scope(
                            [&](){
                                ++resource_factory_invoked;
                                return resource(sc.clock());
                            },
                            [&](resource) -> rx::observable<int> {
                                ++observable_factory_invoked;
                                throw ex;
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
                    on.error(200, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}
