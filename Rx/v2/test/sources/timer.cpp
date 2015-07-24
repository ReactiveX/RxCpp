#include "../test.h"

SCENARIO("timer", "[hide][periodically][timer][scheduler][long][perf][sources]"){
    GIVEN("the timer of 1 sec"){
        WHEN("the period is 1 sec"){
            using namespace std::chrono;

            auto sc = rxsc::make_current_thread();
            auto so = rx::synchronize_in_one_worker(sc);
            auto start = sc.now();
            auto period = seconds(1);
            rx::composite_subscription cs;
            rx::observable<>::timer(period, so)
                .subscribe(
                    cs,
                    [=](long counter){
                        auto nsDelta = duration_cast<milliseconds>(sc.now() - (start + (period * counter)));
                        std::cout << "timer          : period " << counter << ", " << nsDelta.count() << "ms delta from target time" << std::endl;
                    },
                    [](std::exception_ptr){abort();},
                    [](){std::cout << "completed" << std::endl;});
        }
    }
}
