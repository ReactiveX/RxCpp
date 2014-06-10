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


SCENARIO("synchronize concat ranges", "[hide][range][synchronize][concat][perf]"){
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
                .concat(
                    so,
                    rxs::range(sectionCount, sectionCount * 2 - 1, 1, so),
                    rxs::range(sectionCount * 2, onnextcalls - 1, 1, so))
                .subscribe(
                    [&c](int x){
                        ++c;},
                    [](std::exception_ptr){abort();},
                    [&](){
                        wake.notify_one();});

            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&](){return c == onnextcalls;});

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "concat sync ranges : " << n << " subscribed, " << c << " emitted, " << msElapsed.count() << "ms elapsed " << std::endl;
        }
    }
}


SCENARIO("concat completes", "[concat][join][operators]"){
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
                        .concat()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains merged ints"){
                record items[] = {
	                on_next(310, 101),
	                on_next(320, 102),
	                on_next(410, 103),
	                on_next(420, 104),
	                on_next(510, 105),
                    on_next(520, 106),
                    on_next(540, 201),
                    on_next(550, 202),
                    on_next(560, 203),
                    on_next(570, 204),
	                on_next(590, 301),
	                on_next(600, 302),
	                on_next(610, 303),
	                on_next(620, 304),
	                on_next(700, 305),
	                on_completed(730)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
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
                    subscribe(530, 580)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys2.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ys3"){
                life items[] = {
                    subscribe(580, 730)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys3.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
