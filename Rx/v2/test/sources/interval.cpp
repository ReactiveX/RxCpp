#include "../test.h"

SCENARIO("schedule_periodically", "[hide][periodically][scheduler][long][perf][sources]"){
    GIVEN("schedule_periodically"){
        WHEN("the period is 1sec and the initial is 2sec"){
            using namespace std::chrono;

            int c = 0;
            auto sc = rxsc::make_current_thread();
            auto w = sc.create_worker();
            auto start = w.now() + seconds(2);
            auto period = seconds(1);
            w.schedule_periodically(start, period,
                [=, &c](rxsc::schedulable scbl){
                    auto nsDelta = duration_cast<milliseconds>(scbl.now() - (start + (period * c)));
                    ++c;
                    std::cout << "schedule_periodically          : period " << c << ", " << nsDelta.count() << "ms delta from target time" << std::endl;
                    if (c == 5) {scbl.unsubscribe();}
                });
        }
    }
}

SCENARIO("schedule_periodically by duration", "[hide][periodically][scheduler][long][perf][sources]"){
    GIVEN("schedule_periodically_duration"){
        WHEN("the period is 1sec and the initial is 2sec"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int c = 0;
            auto sc = rxsc::make_current_thread();
            auto w = sc.create_worker();

            auto schedule_periodically_duration = [w](
                    rxsc::current_thread::clock_type::duration initial,
                    rxsc::current_thread::clock_type::duration period,
                    rxsc::schedulable activity){
                auto periodic = rxsc::make_schedulable(
                    activity,
                    [period, activity](rxsc::schedulable self) {
                        auto start = clock::now();
                        // any recursion requests will be pushed to the scheduler queue
                        rxsc::recursion r(false);
                        // call action
                        activity(r.get_recurse());
                        auto finish = clock::now();

                        // schedule next occurance (if the action took longer than 'period' target will be in the past)
                        self.schedule(period - (finish - start));
                    });
                w.schedule(initial, periodic);
            };

            auto start = w.now() + seconds(2);
            auto period = seconds(1);
            schedule_periodically_duration(seconds(2), period,
                rxsc::make_schedulable(w, [=, &c](rxsc::schedulable scbl){
                    auto nsDelta = duration_cast<milliseconds>(scbl.now() - (start + (period * c)));
                    ++c;
                    std::cout << "schedule_periodically_duration : period " << c << ", " << nsDelta.count() << "ms delta from target time" << std::endl;
                    if (c == 5) {scbl.unsubscribe();}
                }));
        }
    }
}

SCENARIO("intervals", "[hide][periodically][interval][scheduler][long][perf][sources]"){
    GIVEN("10 intervals of 1 seconds"){
        WHEN("the period is 1sec and the initial is 2sec"){
            using namespace std::chrono;

            int c = 0;
            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now() + seconds(2);
            auto period = seconds(1);
            rx::composite_subscription cs;
            rx::observable<>::interval(start, period, so)
                .subscribe(
                    cs,
                    [=, &c](long counter){
                        auto nsDelta = duration_cast<milliseconds>(sc.now() - (start + (period * (counter - 1))));
                        c = counter - 1;
                        std::cout << "interval          : period " << counter << ", " << nsDelta.count() << "ms delta from target time" << std::endl;
                        if (counter == 5) {cs.unsubscribe();}
                    },
                    [](std::exception_ptr){abort();});
        }
    }
}
