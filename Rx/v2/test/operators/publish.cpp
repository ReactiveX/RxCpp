#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"


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
            on.on_next(110, 7),
            on.on_next(220, 3),
            on.on_next(280, 4),
            on.on_next(290, 1),
            on.on_next(340, 8),
            on.on_next(360, 5),
            on.on_next(370, 6),
            on.on_next(390, 7),
            on.on_next(410, 13),
            on.on_next(430, 2),
            on.on_next(450, 9),
            on.on_next(520, 11),
            on.on_next(560, 20),
            on.on_completed(600)
        });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable& scbl){
                    ys = xs.publish().as_dynamic();
                    //ys = xs.publish_last().as_dynamic();
                });

            w.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable& scbl){
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable& scbl){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                w.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(400,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(550,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(650,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(800,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(340, 8),
                    on.on_next(360, 5),
                    on.on_next(370, 6),
                    on.on_next(390, 7),
                    on.on_next(520, 11)
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
            on.on_next(110, 7),
            on.on_next(220, 3),
            on.on_next(280, 4),
            on.on_next(290, 1),
            on.on_next(340, 8),
            on.on_next(360, 5),
            on.on_next(370, 6),
            on.on_next(390, 7),
            on.on_next(410, 13),
            on.on_next(430, 2),
            on.on_next(450, 9),
            on.on_next(520, 11),
            on.on_next(560, 20),
            on.on_error(600, ex)
        });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable& scbl){
                    ys = xs.publish().as_dynamic();
                });

            w.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable& scbl){
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable& scbl){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                w.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(400,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(800,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(340, 8),
                    on.on_next(360, 5),
                    on.on_next(370, 6),
                    on.on_next(390, 7),
                    on.on_next(520, 11),
                    on.on_next(560, 20),
                    on.on_error(600, ex)
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
                on.on_next(110, 7),
                on.on_next(220, 3),
                on.on_next(280, 4),
                on.on_next(290, 1),
                on.on_next(340, 8),
                on.on_next(360, 5),
                on.on_next(370, 6),
                on.on_next(390, 7),
                on.on_next(410, 13),
                on.on_next(430, 2),
                on.on_next(450, 9),
                on.on_next(520, 11),
                on.on_next(560, 20),
                on.on_completed(600)
            });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable& scbl){
                    ys = xs.publish(1979).as_dynamic();
                });

            w.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable& scbl){
                ys.subscribe(res);
            });

            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable& scbl){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                w.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(400,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(550,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                w.schedule_absolute(650,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                w.schedule_absolute(800,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.on_next(200, 1979),
                    on.on_next(340, 8),
                    on.on_next(360, 5),
                    on.on_next(370, 6),
                    on.on_next(390, 7),
                    on.on_next(520, 11)
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
