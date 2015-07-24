#include "../test.h"

const int static_onnextcalls = 100000;

SCENARIO("range observed on new_thread", "[hide][range][observe_on_debug][observe_on][long][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a range"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto el = rx::observe_on_new_thread();

            for (int n = 0; n < 10; n++)
            {
                std::atomic_bool disposed;
                std::atomic_bool done;
                auto c = std::make_shared<int>(0);

                rx::composite_subscription cs;
                cs.add([&](){
                    if (!done) {abort();}
                    disposed = true;
                });

                auto start = clock::now();
                rxs::range<int>(1)
                    .take(onnextcalls)
                    .observe_on(el)
                    .as_blocking()
                    .subscribe(
                        cs,
                        [c](int){
                           ++(*c);
                        },
                        [&](){
                            done = true;
                        });
                auto expected = onnextcalls;
                REQUIRE(*c == expected);
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "range -> observe_on new_thread : " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed, int-per-second " << *c / (msElapsed.count() / 1000.0) << std::endl;
            }
        }
    }
}
