#include "../test.h"
#include <rxcpp/operators/rx-replay.hpp>

SCENARIO("replay basic", "[replay][multicast][subject][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 0),
            on.next(220, 1),
            on.next(280, 2),
            on.next(290, 3),
            on.next(340, 4),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(430, 9),
            on.next(450, 10),
            on.next(520, 11),
            on.next(560, 12),
            on.completed(600)
        });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.replay().as_dynamic();
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
                    on.next(340, 4),
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

SCENARIO("replay error", "[replay][error][multicast][subject][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("publish on_error");

        auto xs = sc.make_hot_observable({
            on.next(110, 0),
            on.next(220, 1),
            on.next(280, 2),
            on.next(290, 3),
            on.next(340, 4),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(430, 9),
            on.next(450, 10),
            on.next(520, 11),
            on.next(560, 12),
            on.error(600, ex)
        });

        auto res = w.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.replay().as_dynamic();
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
                    on.next(340, 4),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.error(600, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there were 2 subscription/unsubscription"){
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

SCENARIO("replay multiple subscriptions", "[replay][multicast][subject][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 0),
            on.next(220, 1),
            on.next(280, 2),
            on.next(290, 3),
            on.next(340, 4),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(430, 9),
            on.next(450, 10),
            on.next(520, 11),
            on.next(560, 12),
            on.completed(650)
        });

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            // Create connectable observable
            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.replay().as_dynamic();
                });

            // Manage connection
            rx::composite_subscription connection;
            w.schedule_absolute(rxsc::test::subscribed_time,
                [connection, &ys](const rxsc::schedulable&){
                    ys.connect(connection);
                });
            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [connection](const rxsc::schedulable&){
                    connection.unsubscribe();
                });

            // Subscribe before the first item emitted
            auto res1 = w.make_subscriber<int>();
            w.schedule_absolute(200, [&ys, &res1](const rxsc::schedulable&){ys.subscribe(res1);});

            // Subscribe in the middle of emitting
            auto res2 = w.make_subscriber<int>();
            w.schedule_absolute(400, [&ys, &res2](const rxsc::schedulable&){ys.subscribe(res2);});

            // Subscribe after the last item emitted
            auto res3 = w.make_subscriber<int>();
            w.schedule_absolute(600, [&ys, &res3](const rxsc::schedulable&){ys.subscribe(res3);});

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(220, 1),
                    on.next(280, 2),
                    on.next(290, 3),
                    on.next(340, 4),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(400, 1),
                    on.next(400, 2),
                    on.next(400, 3),
                    on.next(400, 4),
                    on.next(400, 5),
                    on.next(400, 6),
                    on.next(400, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(600, 1),
                    on.next(600, 2),
                    on.next(600, 3),
                    on.next(600, 4),
                    on.next(600, 5),
                    on.next(600, 6),
                    on.next(600, 7),
                    on.next(600, 8),
                    on.next(600, 9),
                    on.next(600, 10),
                    on.next(600, 11),
                    on.next(600, 12),
                    on.completed(650)
                });
                auto actual = res3.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 650)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("replay multiple subscriptions with count", "[replay][multicast][subject][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 0),
            on.next(220, 1),
            on.next(280, 2),
            on.next(290, 3),
            on.next(340, 4),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(430, 9),
            on.next(450, 10),
            on.next(520, 11),
            on.next(560, 12),
            on.completed(650)
        });

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            // Create connectable observable
            w.schedule_absolute(rxsc::test::created_time,
                [&ys, &xs](const rxsc::schedulable&){
                    ys = xs.replay(3).as_dynamic();
                });

            // Manage connection
            rx::composite_subscription connection;
            w.schedule_absolute(rxsc::test::subscribed_time,
                [connection, &ys](const rxsc::schedulable&){
                    ys.connect(connection);
                });
            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [connection](const rxsc::schedulable&){
                    connection.unsubscribe();
                });

            // Subscribe before the first item emitted
            auto res1 = w.make_subscriber<int>();
            w.schedule_absolute(200, [&ys, &res1](const rxsc::schedulable&){ys.subscribe(res1);});

            // Subscribe in the middle of emitting
            auto res2 = w.make_subscriber<int>();
            w.schedule_absolute(400, [&ys, &res2](const rxsc::schedulable&){ys.subscribe(res2);});

            // Subscribe after the last item emitted
            auto res3 = w.make_subscriber<int>();
            w.schedule_absolute(600, [&ys, &res3](const rxsc::schedulable&){ys.subscribe(res3);});

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(220, 1),
                    on.next(280, 2),
                    on.next(290, 3),
                    on.next(340, 4),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(400, 5),
                    on.next(400, 6),
                    on.next(400, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(600, 10),
                    on.next(600, 11),
                    on.next(600, 12),
                    on.completed(650)
                });
                auto actual = res3.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 650)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("replay multiple subscriptions with time", "[replay][multicast][subject][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        auto so = rx::identity_one_worker(sc);
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 0),
            on.next(220, 1),
            on.next(240, 2),
            on.next(260, 3),
            on.next(340, 4),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(430, 9),
            on.next(450, 10),
            on.next(520, 11),
            on.next(560, 12),
            on.completed(650)
        });

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){
            using namespace std::chrono;

            // Create connectable observable
            w.schedule_absolute(rxsc::test::created_time,
                [&](const rxsc::schedulable&){
                    ys = xs.replay(milliseconds(100), so).as_dynamic();
                });

            // Manage connection
            rx::composite_subscription connection;
            w.schedule_absolute(rxsc::test::subscribed_time,
                [connection, &ys](const rxsc::schedulable&){
                    ys.connect(connection);
                });
            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [connection](const rxsc::schedulable&){
                    connection.unsubscribe();
                });

            // Subscribe before the first item emitted
            auto res1 = w.make_subscriber<int>();
            w.schedule_absolute(200, [&ys, &res1](const rxsc::schedulable&){ys.subscribe(res1);});

            // Subscribe in the middle of emitting
            auto res2 = w.make_subscriber<int>();
            w.schedule_absolute(400, [&ys, &res2](const rxsc::schedulable&){ys.subscribe(res2);});

            // Subscribe after the last item emitted
            auto res3 = w.make_subscriber<int>();
            w.schedule_absolute(600, [&ys, &res3](const rxsc::schedulable&){ys.subscribe(res3);});

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(220, 1),
                    on.next(240, 2),
                    on.next(260, 3),
                    on.next(340, 4),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(400, 4),
                    on.next(400, 5),
                    on.next(400, 6),
                    on.next(400, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(600, 11),
                    on.next(600, 12),
                    on.completed(650)
                });
                auto actual = res3.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 650)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("replay multiple subscriptions with count and time", "[replay][multicast][subject][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        auto so = rx::identity_one_worker(sc);
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(110, 0),
            on.next(220, 1),
            on.next(240, 2),
            on.next(260, 3),
            on.next(340, 4),
            on.next(360, 5),
            on.next(370, 6),
            on.next(390, 7),
            on.next(410, 8),
            on.next(430, 9),
            on.next(450, 10),
            on.next(520, 11),
            on.next(560, 12),
            on.completed(650)
        });

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){
            using namespace std::chrono;

            // Create connectable observable
            w.schedule_absolute(rxsc::test::created_time,
                [&](const rxsc::schedulable&){
                    ys = xs.replay(3, milliseconds(100), so).as_dynamic();
                });

            // Manage connection
            rx::composite_subscription connection;
            w.schedule_absolute(rxsc::test::subscribed_time,
                [connection, &ys](const rxsc::schedulable&){
                    ys.connect(connection);
                });
            w.schedule_absolute(rxsc::test::unsubscribed_time,
                [connection](const rxsc::schedulable&){
                    connection.unsubscribe();
                });

            // Subscribe before the first item emitted
            auto res1 = w.make_subscriber<int>();
            w.schedule_absolute(200, [&ys, &res1](const rxsc::schedulable&){ys.subscribe(res1);});

            // Subscribe in the middle of emitting
            auto res2 = w.make_subscriber<int>();
            w.schedule_absolute(400, [&ys, &res2](const rxsc::schedulable&){ys.subscribe(res2);});

            // Subscribe after the last item emitted
            auto res3 = w.make_subscriber<int>();
            w.schedule_absolute(600, [&ys, &res3](const rxsc::schedulable&){ys.subscribe(res3);});

            w.start();

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(220, 1),
                    on.next(240, 2),
                    on.next(260, 3),
                    on.next(340, 4),
                    on.next(360, 5),
                    on.next(370, 6),
                    on.next(390, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(400, 5),
                    on.next(400, 6),
                    on.next(400, 7),
                    on.next(410, 8),
                    on.next(430, 9),
                    on.next(450, 10),
                    on.next(520, 11),
                    on.next(560, 12),
                    on.completed(650)
                });
                auto actual = res2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(600, 11),
                    on.next(600, 12),
                    on.completed(650)
                });
                auto actual = res3.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 650)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
