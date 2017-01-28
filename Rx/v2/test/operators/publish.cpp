#include "../test.h"
#include <rxcpp/operators/rx-publish.hpp>
#include <rxcpp/operators/rx-connect_forever.hpp>
#include <rxcpp/operators/rx-ref_count.hpp>


SCENARIO("publish range", "[hide][range][subject][publish][subject][operators]"){
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
