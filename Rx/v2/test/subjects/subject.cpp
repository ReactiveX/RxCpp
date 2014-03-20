
#define RXCPP_SUBJECT_TEST_ASYNC 1

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


SCENARIO("subject test", "[hide][subject][subjects][perf]"){
    GIVEN("a subject"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            const int onnextcalls = 10000000;

            {
                int c = 0;
                int n = 1;
                auto o = rx::make_observer<int>([&c](int){++c;});
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop no subject     : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                int c = 0;
                int n = 1;
                auto start = clock::now();
                rxs::range<int>(0, onnextcalls).subscribe([&c](int){++c;});
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "range no subject    : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            {
                std::mutex m;
                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    std::unique_lock<std::mutex> guard(m);
                    ++c;
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop mutex          : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            for (int n = 0; n < 10; n++)
            {
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                auto o = sub.get_subscriber();

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, o]() {
                        auto source = sub.get_observable();
                        while(o.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            rx::composite_subscription cs;
                            source.subscribe(cs, [cs](int){
                                cs.unsubscribe();
                            });
                        }
                        return 0;
                    });
#endif
                    sub.get_observable().subscribe([c](int){++(*c);});
                }

                auto start = clock::now();
                for (int i = 0; i < onnextcalls; i++) {
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "loop                : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }

            for (int n = 0; n < 10; n++)
            {
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                auto o = sub.get_subscriber();

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, o]() {
                        while(o.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            rx::composite_subscription cs;
                            sub.get_observable().subscribe(cs, [cs](int){
                                cs.unsubscribe();
                            });
                        }
                        return 0;
                    });
#endif
                    sub.get_observable()
                        // demonstrates insertion of user definied operator
                        .op(rxo::filter([](int){return true;}))
                        .subscribe([c](int){++(*c);});
                }

                auto start = clock::now();
                rxs::range<int>(0, onnextcalls).subscribe(o);
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                       duration_cast<milliseconds>(start.time_since_epoch());
                std::cout << "range               : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << std::endl;
            }
        }
    }
}



SCENARIO("subject - infinite source", "[subject][subjects]"){
    GIVEN("a subject and an infinite source"){

        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        m::recorded_type messages[] = {
            m::on_next(70, 1),
            m::on_next(110, 2),
            m::on_next(220, 3),
            m::on_next(270, 4),
            m::on_next(340, 5),
            m::on_next(410, 6),
            m::on_next(520, 7),
            m::on_next(630, 8),
            m::on_next(710, 9),
            m::on_next(870, 10),
            m::on_next(940, 11),
            m::on_next(1020, 12)
        };
        auto xs = sc->make_hot_observable(messages);

        rxsub::subject<int> s;

        auto results1 = sc->make_subscriber<int>();

        auto results2 = sc->make_subscriber<int>();

        auto results3 = sc->make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            sc->schedule_absolute(100, [&s](rxsc::action, rxsc::scheduler){
                s = rxsub::subject<int>(); return rxsc::make_action_empty();});
            sc->schedule_absolute(200, [&xs, &o](rxsc::action, rxsc::scheduler){
                xs.subscribe(o); return rxsc::make_action_empty();});
            sc->schedule_absolute(1000, [&o](rxsc::action, rxsc::scheduler){
                o.unsubscribe(); return rxsc::make_action_empty();});

            sc->schedule_absolute(300, [&s, &results1](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results1); return rxsc::make_action_empty();});
            sc->schedule_absolute(400, [&s, &results2](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results2); return rxsc::make_action_empty();});
            sc->schedule_absolute(900, [&s, &results3](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results3); return rxsc::make_action_empty();});

            sc->schedule_absolute(600, [&results1](rxsc::action, rxsc::scheduler){
                results1.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(700, [&results2](rxsc::action, rxsc::scheduler){
                results2.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(800, [&results1](rxsc::action, rxsc::scheduler){
                results1.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(950, [&results3](rxsc::action, rxsc::scheduler){
                results3.unsubscribe(); return rxsc::make_action_empty();});

            sc->start();

            THEN("result1 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(340, 5),
                    m::on_next(410, 6),
                    m::on_next(520, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(410, 6),
                    m::on_next(520, 7),
                    m::on_next(630, 8)
                };
                auto required = rxu::to_vector(items);
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(940, 11)
                };
                auto required = rxu::to_vector(items);
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("subject - finite source", "[subject][subjects]"){
    GIVEN("a subject and an finite source"){

        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        m::recorded_type messages[] = {
            m::on_next(70, 1),
            m::on_next(110, 2),
            m::on_next(220, 3),
            m::on_next(270, 4),
            m::on_next(340, 5),
            m::on_next(410, 6),
            m::on_next(520, 7),
            m::on_completed(630),
            m::on_next(640, 9),
            m::on_completed(650),
            m::on_error(660, std::runtime_error("error on unsubscribed stream"))
        };
        auto xs = sc->make_hot_observable(messages);

        rxsub::subject<int> s;

        auto results1 = sc->make_subscriber<int>();

        auto results2 = sc->make_subscriber<int>();

        auto results3 = sc->make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            sc->schedule_absolute(100, [&s](rxsc::action, rxsc::scheduler){
                s = rxsub::subject<int>(); return rxsc::make_action_empty();});
            sc->schedule_absolute(200, [&xs, &o](rxsc::action, rxsc::scheduler){
                xs.subscribe(o); return rxsc::make_action_empty();});
            sc->schedule_absolute(1000, [&o](rxsc::action, rxsc::scheduler){
                o.unsubscribe(); return rxsc::make_action_empty();});

            sc->schedule_absolute(300, [&s, &results1](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results1); return rxsc::make_action_empty();});
            sc->schedule_absolute(400, [&s, &results2](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results2); return rxsc::make_action_empty();});
            sc->schedule_absolute(900, [&s, &results3](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results3); return rxsc::make_action_empty();});

            sc->schedule_absolute(600, [&results1](rxsc::action, rxsc::scheduler){
                results1.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(700, [&results2](rxsc::action, rxsc::scheduler){
                results2.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(800, [&results1](rxsc::action, rxsc::scheduler){
                results1.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(950, [&results3](rxsc::action, rxsc::scheduler){
                results3.unsubscribe(); return rxsc::make_action_empty();});

            sc->start();

            THEN("result1 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(340, 5),
                    m::on_next(410, 6),
                    m::on_next(520, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(410, 6),
                    m::on_next(520, 7),
                    m::on_completed(630)
                };
                auto required = rxu::to_vector(items);
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_completed(900)
                };
                auto required = rxu::to_vector(items);
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}


SCENARIO("subject - on_error in source", "[subject][subjects]"){
    GIVEN("a subject and a source with an error"){

        auto sc = std::make_shared<rxsc::test>();
        typedef rxsc::test::messages<int> m;

        std::runtime_error ex("subject on_error in stream");

        m::recorded_type messages[] = {
            m::on_next(70, 1),
            m::on_next(110, 2),
            m::on_next(220, 3),
            m::on_next(270, 4),
            m::on_next(340, 5),
            m::on_next(410, 6),
            m::on_next(520, 7),
            m::on_error(630, ex),
            m::on_next(640, 9),
            m::on_completed(650),
            m::on_error(660, std::runtime_error("error on unsubscribed stream"))
        };
        auto xs = sc->make_hot_observable(messages);

        rxsub::subject<int> s;

        auto results1 = sc->make_subscriber<int>();

        auto results2 = sc->make_subscriber<int>();

        auto results3 = sc->make_subscriber<int>();

        WHEN("multicasting an infinite source"){


            auto o = s.get_subscriber();

            sc->schedule_absolute(100, [&s](rxsc::action, rxsc::scheduler){
                s = rxsub::subject<int>(); return rxsc::make_action_empty();});
            sc->schedule_absolute(200, [&xs, &o](rxsc::action, rxsc::scheduler){
                xs.subscribe(o); return rxsc::make_action_empty();});
            sc->schedule_absolute(1000, [&o](rxsc::action, rxsc::scheduler){
                o.unsubscribe(); return rxsc::make_action_empty();});

            sc->schedule_absolute(300, [&s, &results1](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results1); return rxsc::make_action_empty();});
            sc->schedule_absolute(400, [&s, &results2](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results2); return rxsc::make_action_empty();});
            sc->schedule_absolute(900, [&s, &results3](rxsc::action, rxsc::scheduler){
                s.get_observable().subscribe(results3); return rxsc::make_action_empty();});

            sc->schedule_absolute(600, [&results1](rxsc::action, rxsc::scheduler){
                results1.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(700, [&results2](rxsc::action, rxsc::scheduler){
                results2.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(800, [&results1](rxsc::action, rxsc::scheduler){
                results1.unsubscribe(); return rxsc::make_action_empty();});
            sc->schedule_absolute(950, [&results3](rxsc::action, rxsc::scheduler){
                results3.unsubscribe(); return rxsc::make_action_empty();});

            sc->start();

            THEN("result1 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(340, 5),
                    m::on_next(410, 6),
                    m::on_next(520, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_next(410, 6),
                    m::on_next(520, 7),
                    m::on_error(630, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                m::recorded_type items[] = {
                    m::on_error(900, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}
