#include "../test.h"

static const int static_subscriptions = 50000;

SCENARIO("for loop subscribes to map with subscribe_on and observe_on", "[hide][for][just][subscribe][subscribe_on][observe_on][long][perf]"){
    const int& subscriptions = static_subscriptions;
    GIVEN("a for loop"){
        WHEN("subscribe 50K times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int runs = 10;

            for (;runs > 0; --runs) {

                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < subscriptions; ++i) {
                    c += rx::observable<>::just(1)
                        .map([](int i) {
                            std::stringstream serializer;
                            serializer << i;
                            return serializer.str();
                        })
                        .map([](const std::string& s) {
                            int i;
                            std::stringstream(s) >> i;
                            return i;
                        })
                        .subscribe_on(rx::observe_on_event_loop())
                        .observe_on(rx::observe_on_event_loop())
                        .as_blocking()
                        .count();
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                REQUIRE(subscriptions == c);
                std::cout << "loop subscribe map subscribe_on observe_on : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
            }
        }
    }
}

SCENARIO("for loop subscribes to map with subscribe_on", "[hide][subscribe_on_only][for][just][subscribe][subscribe_on][long][perf]"){
    const int& subscriptions = static_subscriptions;
    GIVEN("a for loop"){
        WHEN("subscribe 50K times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int runs = 10;

            for (;runs > 0; --runs) {

                int c = 0;
                int n = 1;
                auto start = clock::now();

                for (int i = 0; i < subscriptions; ++i) {
                    c += rx::observable<>::
                        just(1).
                        map([](int i) {
                            std::stringstream serializer;
                            serializer << i;
                            return serializer.str();
                        }).
                        map([](const std::string& s) {
                            int i;
                            std::stringstream(s) >> i;
                            return i;
                        }).
                        subscribe_on(rx::observe_on_event_loop()).
                        as_blocking().
                        count();
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                REQUIRE(subscriptions == c);
                std::cout << "loop subscribe map subscribe_on : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
            }
        }
    }
}
