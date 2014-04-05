#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxo=rxcpp::operators;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;
namespace rxsub=rxcpp::subjects;
namespace rxn=rxcpp::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt=rxcpp::test;

#include "catch.hpp"


SCENARIO("publish range", "[hide][range][subject][publish][operators]"){
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
        WHEN("connect_now is used"){
            auto published = rxs::range<int>(0, 10).publish().connect_now();
            std::cout << "subscribe to connect_now" << std::endl;
            published.subscribe(
            // on_next
                [](int v){std::cout << v << ", ";},
            // on_completed
                [](){std::cout << " done." << std::endl;});
        }
    }
}

SCENARIO("publish", "[publish][multicast][operators]"){
    GIVEN("a test hot observable of longs"){
        auto sc = rxsc::make_test();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 7),
            on_next(220, 3),
            on_next(280, 4),
            on_next(290, 1),
            on_next(340, 8),
            on_next(360, 5),
            on_next(370, 6),
            on_next(390, 7),
            on_next(410, 13),
            on_next(430, 2),
            on_next(450, 9),
            on_next(520, 11),
            on_next(560, 20),
            on_completed(600)
        };
        auto xs = sc.make_hot_observable(messages);

        auto res = sc.make_subscriber<int>();

        rx::connectable_observable<int> ys;

        WHEN("subscribed and then connected"){

            sc.schedule_absolute(rxsc::test::created_time,
                [&invoked, &ys, &xs](const rxsc::schedulable& scbl){
                    ys = xs.publish().as_dynamic();
                    //ys = xs.publish_last().as_dynamic();
                });

            sc.schedule_absolute(rxsc::test::subscribed_time,
                [&ys, &res](const rxsc::schedulable& scbl){
                ys.subscribe(res);
            });

            sc.schedule_absolute(rxsc::test::unsubscribed_time,
                [&res](const rxsc::schedulable& scbl){
                    res.unsubscribe();
                });

            {
                rx::composite_subscription connection;

                sc.schedule_absolute(300,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                sc.schedule_absolute(400,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                sc.schedule_absolute(500,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                sc.schedule_absolute(550,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            {
                rx::composite_subscription connection;

                sc.schedule_absolute(650,
                    [connection, &ys](const rxsc::schedulable& scbl){
                        ys.connect(connection);
                    });
                sc.schedule_absolute(800,
                    [connection](const rxsc::schedulable& scbl){
                        connection.unsubscribe();
                    });
            }

            sc.start();

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(340, 8),
                    on_next(360, 5),
                    on_next(370, 6),
                    on_next(390, 7),
                    on_next(520, 11)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there were 3 subscription/unsubscription"){
                life items[] = {
                    subscribe(300, 400),
                    subscribe(500, 550),
                    subscribe(650, 800)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

