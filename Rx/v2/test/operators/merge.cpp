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

const int static_onnextcalls = 1000000;


SCENARIO("synchronize merge ranges", "[hide][range][synchronize][merge][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("some ranges"){
        WHEN("generating ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            std::mutex lock;
            std::condition_variable wake;

            auto sc = rxsc::make_event_loop();
            //auto sc = rxsc::make_new_thread();
            auto so = rx::syncronize_in_one_worker(sc);

            std::atomic<int> c(0);
            int n = 1;
            auto sectionCount = onnextcalls / 3;
            auto start = clock::now();
            rxs::range(0, sectionCount - 1, 1, so)
                .merge(
                    so,
                    rxs::range(sectionCount, (sectionCount * 2) - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .subscribe(
                    [&c](int x){
                        ++c;},
                    [](std::exception_ptr){abort();},
                    [&](){
                        wake.notify_one();
                    });

            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&](){return c == onnextcalls;});

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge sync ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}

SCENARIO("merge completes", "[merge][join][operators]"){
    GIVEN("1 hot observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<rx::observable<int>> mo;
        typedef rxn::subscription life;

        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        typedef mo::recorded_type orecord;
        auto oon_next = mo::on_next;
        auto oon_completed = mo::on_completed;
        auto osubscribe = mo::subscribe;

        record messages1[] = {
            on_next(10, 101),
            on_next(20, 102),
            on_next(110, 103),
            on_next(120, 104),
            on_next(210, 105),
            on_next(220, 106),
            on_completed(230)
        };
        auto ys1 = sc.make_cold_observable(messages1);

        record messages2[] = {
            on_next(10, 201),
            on_next(20, 202),
            on_next(30, 203),
            on_next(40, 204),
            on_completed(50)
        };
        auto ys2 = sc.make_cold_observable(messages2);

        record messages3[] = {
            on_next(10, 301),
            on_next(20, 302),
            on_next(30, 303),
            on_next(40, 304),
            on_next(120, 305),
            on_completed(150)
        };
        auto ys3 = sc.make_cold_observable(messages3);

        orecord omessages[] = {
            oon_next(300, ys1),
            oon_next(400, ys2),
            oon_next(500, ys3),
            oon_completed(600)
        };
        auto xs = sc.make_hot_observable(omessages);

        WHEN("each int is merged"){

            auto res = w.start<int>(
                [&]() {
                    return xs
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged ints"){
                record items[] = {
	                on_next(310, 101),
	                on_next(320, 102),
	                on_next(410, 103),
	                on_next(410, 201),
	                on_next(420, 104),
	                on_next(420, 202),
	                on_next(430, 203),
	                on_next(440, 204),
	                on_next(510, 105),
	                on_next(510, 301),
	                on_next(520, 106),
	                on_next(520, 302),
	                on_next(530, 303),
	                on_next(540, 304),
	                on_next(620, 305),
	                on_completed(650)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                life items[] = {
                    subscribe(200, 600)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                life items[] = {
                    subscribe(300, 530)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                life items[] = {
                    subscribe(400, 450)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                life items[] = {
                    subscribe(500, 650)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("variadic merge completes", "[merge][join][operators]"){
    GIVEN("1 hot observable with 3 cold observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<rx::observable<int>> mo;
        typedef rxn::subscription life;

        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        typedef mo::recorded_type orecord;
        auto oon_next = mo::on_next;
        auto oon_completed = mo::on_completed;
        auto osubscribe = mo::subscribe;

        record messages1[] = {
            on_next(10, 101),
            on_next(20, 102),
            on_next(110, 103),
            on_next(120, 104),
            on_next(210, 105),
            on_next(220, 106),
            on_completed(230)
        };
        auto ys1 = sc.make_cold_observable(messages1);

        record messages2[] = {
            on_next(10, 201),
            on_next(20, 202),
            on_next(30, 203),
            on_next(40, 204),
            on_completed(50)
        };
        auto ys2 = sc.make_cold_observable(messages2);

        record messages3[] = {
            on_next(10, 301),
            on_next(20, 302),
            on_next(30, 303),
            on_next(40, 304),
            on_next(120, 305),
            on_completed(150)
        };
        auto ys3 = sc.make_cold_observable(messages3);

        WHEN("each int is merged"){

            auto res = w.start<int>(
                [&]() {
                    return ys1
                        .merge(ys2, ys3);
                }
            );

            THEN("the output contains merged ints"){
                record items[] = {
                    on_next(210, 101),
                    on_next(210, 201),
                    on_next(210, 301),
                    on_next(220, 102),
                    on_next(220, 202),
                    on_next(220, 302),
                    on_next(230, 203),
                    on_next(230, 303),
                    on_next(240, 204),
                    on_next(240, 304),
                    on_next(310, 103),
                    on_next(320, 104),
                    on_next(320, 305),
                    on_next(410, 105),
                    on_next(420, 106),
                    on_completed(430)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys1"){
                life items[] = {
                    subscribe(200, 430)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys2"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                life items[] = {
                    subscribe(200, 350)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
