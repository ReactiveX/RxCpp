#include "../test.h"
#include <rxcpp/operators/rx-publish.hpp>
#include <rxcpp/operators/rx-connect_forever.hpp>
#include <rxcpp/operators/rx-ref_count.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-merge.hpp>


SCENARIO("publish range", "[!hide][range][subject][publish][subject][operators]"){
    GIVEN("a range"){
        WHEN("published"){
            auto published = rxs::range<int>(0, 10).publish();
            std::cout << "subscribe to published" << std::endl;
            published.subscribe(
            // on_next
                [](int v){std::cout << v << ", ";},
            // on_completed
                [](){std::cout << " done." << std::endl;});
            std::cout << "connect to published" << std::endl;
            published.connect();
        }
        WHEN("ref_count is used"){
            auto published = rxs::range<int>(0, 10).publish().ref_count();
            std::cout << "subscribe to ref_count" << std::endl;
            published.subscribe(
            // on_next
                [](int v){std::cout << v << ", ";},
            // on_completed
                [](){std::cout << " done." << std::endl;});
        }
        WHEN("connect_forever is used"){
            auto published = rxs::range<int>(0, 10).publish().connect_forever();
            std::cout << "subscribe to connect_forever" << std::endl;
            published.subscribe(
            // on_next
                [](int v){std::cout << v << ", ";},
            // on_completed
                [](){std::cout << " done." << std::endl;});
        }
    }
}

SCENARIO("publish ref_count", "[range][subject][publish][ref_count][operators]"){
    GIVEN("a range"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<double> on;
        const rxsc::test::messages<long> out;

        static const long start_created = 0;
        static const long start_subscribed = 100;
        static const long start_unsubscribed = 200;

        static const auto next_time = start_subscribed + 1;
        static const auto completed_time = next_time + 1;

        auto xs = sc.make_hot_observable({ // [0.0, 10.0]
            on.next(next_time, 0.0),
            on.next(next_time, 1.0),
            on.next(next_time, 2.0),
            on.next(next_time, 3.0),
            on.next(next_time, 4.0),
            on.next(next_time, 5.0),
            on.next(next_time, 6.0),
            on.next(next_time, 7.0),
            on.next(next_time, 8.0),
            on.next(next_time, 9.0),
            on.next(next_time, 10.0),
            on.completed(completed_time)
        });

        auto xs3 = sc.make_hot_observable({ // [0.0, 3.0]
            on.next(next_time, 0.0),
            on.next(next_time, 1.0),
            on.next(next_time, 2.0),
            on.next(next_time, 3.0),
            on.completed(completed_time)
        });

        WHEN("ref_count is used"){
            auto res = w.start(
                [&xs]() {
                    return xs
                        .publish()
                        .ref_count();
                },
                start_created,
                start_subscribed,
                start_unsubscribed
            );

            THEN("the output contains exactly the input") {
                auto required = rxu::to_vector({
                    on.next(next_time, 0.0),
                    on.next(next_time, 1.0),
                    on.next(next_time, 2.0),
                    on.next(next_time, 3.0),
                    on.next(next_time, 4.0),
                    on.next(next_time, 5.0),
                    on.next(next_time, 6.0),
                    on.next(next_time, 7.0),
                    on.next(next_time, 8.0),
                    on.next(next_time, 9.0),
                    on.next(next_time, 10.0),
                    on.completed(completed_time)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
        WHEN("ref_count(other) is used"){
            auto res = w.start(
                [&xs]() {
                    auto published = xs.publish();

                    auto map_to_int = published.map([](double v) { return (long) v; });

                    // Ensures that 'ref_count(other)' has the source value type,
                    // not the publisher's value type.
                    auto with_ref_count = map_to_int.ref_count(published);

                    return with_ref_count;
                },
                start_created,
                start_subscribed,
                start_unsubscribed
            );

            THEN("the output contains the long-ified input") {
                auto required = rxu::to_vector({
                    out.next(next_time, 0L),
                    out.next(next_time, 1L),
                    out.next(next_time, 2L),
                    out.next(next_time, 3L),
                    out.next(next_time, 4L),
                    out.next(next_time, 5L),
                    out.next(next_time, 6L),
                    out.next(next_time, 7L),
                    out.next(next_time, 8L),
                    out.next(next_time, 9L),
                    out.next(next_time, 10L),
                    out.completed(completed_time)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
        WHEN("ref_count(other) is used in a diamond"){
            auto source = rxs::range<double>(0, 3);

            int published_on_next_count = 0;

            auto res = w.start(
                [&xs3, &published_on_next_count]() {
                    // Ensure we only subscribe once to 'published' when its in a diamond.
                    auto next = xs3.map(
                        [&](double v) {
                            published_on_next_count++;
                            return v;
                        }
                    );

                    auto published = next.publish();

                    // Ensures that 'x.ref_count(other)' has the 'x' value type, not the other's
                    // value type.
                    auto map_to_int = published.map([](double v) { return (long) v; });

                    auto left = map_to_int.map([](long v) { return v * 2; });
                    auto right = map_to_int.map([](long v) { return v * 100; });

                    auto merge = left.merge(right);
                    auto with_ref_count = merge.ref_count(published);

                    return with_ref_count;
                },
                start_created,
                start_subscribed,
                start_unsubscribed
            );

            THEN("the output is subscribed to only once when its in a diamond") {
              // Ensure we only subscribe once to 'published' when its in a diamond.
              CHECK(published_on_next_count == 4);
            }

            THEN("the output left,right is interleaved without being biased towards one side.") {
                auto required = rxu::to_vector({
                    out.next(next_time, 0L),
                    out.next(next_time, 0L),
                    out.next(next_time, 2L),
                    out.next(next_time, 100L),
                    out.next(next_time, 4L),
                    out.next(next_time, 200L),
                    out.next(next_time, 6L),
                    out.next(next_time, 300L),
                    out.completed(completed_time)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("publish basic", "[publish][multicast][subject][operators]"){
    GIVEN("a test hot observable of longs"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 7),
            on.next(220, 3),
            on.next(280, 4),
            on.next(290, 1),
            on.next(340, 8),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 13),
            on.next(430, 2),
            on.next(450, 9),
            on.next(520, 11),
            on.next(560, 20),
            on.completed(600)
        });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.publish().as_dynamic();
                    //ys = xs.publish_last().as_dynamic();
                });

            w.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable&){
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable&){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                w.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(400,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(550,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(650,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(800,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(340, 8),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(520, 11)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there were 3 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400),
                    on.subscribe(500, 550),
                    on.subscribe(650, 800)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}


SCENARIO("publish error", "[publish][error][multicast][subject][operators]"){
    GIVEN("a test hot observable of longs"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("publish on_error");

        auto xs = sc.make_hot_observable({
            on.next(110, 7),
            on.next(220, 3),
            on.next(280, 4),
            on.next(290, 1),
            on.next(340, 8),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 13),
            on.next(430, 2),
            on.next(450, 9),
            on.next(520, 11),
            on.next(560, 20),
            on.error(600, ex)
        });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.publish().as_dynamic();
                });

            w.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable&){
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable&){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                w.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(400,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(800,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(340, 8),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(520, 11),
                    on.next(560, 20),
                    on.error(600, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there were 3 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400),
                    on.subscribe(500, 600)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("publish basic with initial value", "[publish][multicast][behavior][operators]"){
    GIVEN("a test hot observable of longs"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
                on.next(110, 7),
                on.next(220, 3),
                on.next(280, 4),
                on.next(290, 1),
                on.next(340, 8),
                on.next(360, 5),
                on.next(370, 6),
                on.next(390, 7),
                on.next(410, 13),
                on.next(430, 2),
                on.next(450, 9),
                on.next(520, 11),
                on.next(560, 20),
                on.completed(600)
            });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.publish(1979).as_dynamic();
                });

            w.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable&){
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable&){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                w.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(400,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(550,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(650,
                    [connection, &ys](const rxsc::schedulable&){
                        ys.connect(connection);
                    });
                w.schedule_absolute(800,
                    [connection](const rxsc::schedulable&){
                        connection.unsubscribe();
                    });
            }

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(200, 1979),
                    on.next(340, 8),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(520, 11)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there were 3 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(300, 400),
                    on.subscribe(500, 550),
                    on.subscribe(650, 800)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
