#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxs=rx::rxs;
namespace rxsc=rx::rxsc;

#include "catch.hpp"

static const int static_subscriptions = 100000;

SCENARIO("for loop subscribes to map", "[hide][for][just][subscribe][long][perf]"){
    const int& subscriptions = static_subscriptions;
    GIVEN("a for loop"){
        WHEN("subscribe 100K times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto sc = rxsc::make_current_thread();
            auto w = sc.create_worker();
            int runs = 10;

            auto loop = [&](const rxsc::schedulable& self) {
                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < subscriptions; i++) {
                    rx::observable<>::just(1)
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
                        .subscribe([&](int i){
                            ++c;
                        });
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "loop subscribe map             : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

                if (--runs > 0) {
                    self();
                }
            };

            w.schedule(loop);
        }
    }
}

SCENARIO("for loop subscribes to combine_latest", "[hide][for][just][combine_latest][subscribe][long][perf]"){
    const int& subscriptions = static_subscriptions;
    GIVEN("a for loop"){
        WHEN("subscribe 100K times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto sc = rxsc::make_current_thread();
            auto w = sc.create_worker();
            int runs = 10;

            auto loop = [&](const rxsc::schedulable& self) {
                int c = 0;
                int n = 1;
                auto start = clock::now();
                for (int i = 0; i < subscriptions; i++) {
                    rx::observable<>::just(1)
                        .combine_latest([](int i, int j) {
                            return i + j;
                        }, rx::observable<>::just(2))
                        .subscribe([&](int i){
                            ++c;
                        });
                }
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "loop subscribe combine_latest  : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

                if (--runs > 0) {
                    self();
                }
            };

            w.schedule(loop);
        }
    }
}

SCENARIO("synchronized range debug", "[hide][subscribe][range][synchronize_debug][synchronize][long][perf]"){
    GIVEN("range"){
        WHEN("synchronized"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto sc = rxsc::make_current_thread();
            auto w = sc.create_worker();

            auto es = rx::synchronize_event_loop();

            const int values = 10000;

            int runs = 10;

            auto loop = [&](const rxsc::schedulable& self) {
                std::atomic<int> c(0);
                int n = 1;
                auto liftrequirecompletion = [&](rx::subscriber<int> dest){
                    auto completionstate = std::make_shared<std::tuple<bool, long, rx::subscriber<int>>>(false, 0, std::move(dest));
                    std::get<2>(*completionstate).add([=](){
                        if (std::get<1>(*completionstate) != values || !std::get<0>(*completionstate)) {
                            abort();
                        }
                    });
                    // VS2013 deduction issue requires dynamic (type-forgetting)
                    return rx::make_subscriber<int>(
                        std::get<2>(*completionstate),
                        [=](int n){
                            ++std::get<1>(*completionstate);
                            std::get<2>(*completionstate).on_next(n);
                        },
                        [=](std::exception_ptr e){
                            abort();
                            std::get<2>(*completionstate).on_error(e);
                        },
                        [=](){
                            if (std::get<1>(*completionstate) != values) {
                                abort();
                            }
                            std::get<0>(*completionstate) = true;
                            std::get<2>(*completionstate).on_completed();
                        }).as_dynamic();
                };
                auto start = clock::now();
                auto ew = es.create_coordinator().get_worker();
                std::atomic<int> v(0);
                auto s0 = rxs::range(1, es)
                    .take(values)
                    .lift<int>(liftrequirecompletion)
                    .as_dynamic()
                    .publish_synchronized(es)
                    .ref_count()
                    .lift<int>(liftrequirecompletion)
                    .subscribe(
                        rx::make_observer_dynamic<int>(
                        [&](int i){
                            ++v;
                        },
                        [&](){
                            ++c;
                        }));
                auto s1 = rxs::range(values + 1, es)
                    .take(values)
                    .lift<int>(liftrequirecompletion)
                    .as_dynamic()
                    .publish_synchronized(es)
                    .ref_count()
                    .lift<int>(liftrequirecompletion)
                    .subscribe(
                        rx::make_observer_dynamic<int>(
                        [&](int i){
                            ++v;
                        },
                        [&](){
                            ++c;
                        }));
                auto s2 = rxs::range((values * 2) + 1, es)
                    .take(values)
                    .lift<int>(liftrequirecompletion)
                    .as_dynamic()
                    .publish_synchronized(es)
                    .ref_count()
                    .lift<int>(liftrequirecompletion)
                    .subscribe(
                        rx::make_observer_dynamic<int>(
                        [&](int i){
                            ++v;
                        },
                        [&](){
                            ++c;
                        }));
                while(v != values * 3 || c != 3);
                s0.unsubscribe();
                s1.unsubscribe();
                s2.unsubscribe();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "range synchronized : " << n << " subscribed, " << v << " on_next calls, " << msElapsed.count() << "ms elapsed, " << v / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

                if (--runs > 0) {
                    self();
                }
            };

            w.schedule(loop);
        }
    }
}

SCENARIO("observe_on range debug", "[hide][subscribe][range][observe_on_debug][observe_on][long][perf]"){
    GIVEN("range"){
        WHEN("observed on"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto sc = rxsc::make_current_thread();
            auto w = sc.create_worker();

            auto es = rx::observe_on_event_loop();

            const int values = 10000;

            int runs = 10;

            auto loop = [&](const rxsc::schedulable& self) {
                std::atomic<int> c(0);
                int n = 1;
                auto liftrequirecompletion = [&](rx::subscriber<int> dest){
                    auto completionstate = std::make_shared<std::tuple<bool, long, rx::subscriber<int>>>(false, 0, std::move(dest));
                    std::get<2>(*completionstate).add([=](){
                        if (std::get<1>(*completionstate) != values || !std::get<0>(*completionstate)) {
                            abort();
                        }
                    });
                    // VS2013 deduction issue requires dynamic (type-forgetting)
                    return rx::make_subscriber<int>(
                        std::get<2>(*completionstate),
                        [=](int n){
                            ++std::get<1>(*completionstate);
                            std::get<2>(*completionstate).on_next(n);
                        },
                        [=](std::exception_ptr e){
                            abort();
                            std::get<2>(*completionstate).on_error(e);
                        },
                        [=](){
                            if (std::get<1>(*completionstate) != values) {
                                abort();
                            }
                            std::get<0>(*completionstate) = true;
                            std::get<2>(*completionstate).on_completed();
                        }).as_dynamic();
                };
                auto start = clock::now();
                auto ew = es.create_coordinator().get_worker();
                std::atomic<int> v(0);
                auto s0 = rxs::range(1, es)
                    .take(values)
                    .lift<int>(liftrequirecompletion)
                    .as_dynamic()
                    .observe_on(es)
                    .lift<int>(liftrequirecompletion)
                    .subscribe(
                        rx::make_observer_dynamic<int>(
                        [&](int i){
                            ++v;
                        },
                        [&](){
                            ++c;
                        }));
                auto s1 = rxs::range(values + 1, es)
                    .take(values)
                    .lift<int>(liftrequirecompletion)
                    .as_dynamic()
                    .observe_on(es)
                    .lift<int>(liftrequirecompletion)
                    .subscribe(
                        rx::make_observer_dynamic<int>(
                        [&](int i){
                            ++v;
                        },
                        [&](){
                            ++c;
                        }));
                auto s2 = rxs::range((values * 2) + 1, es)
                    .take(values)
                    .lift<int>(liftrequirecompletion)
                    .as_dynamic()
                    .observe_on(es)
                    .lift<int>(liftrequirecompletion)
                    .subscribe(
                        rx::make_observer_dynamic<int>(
                        [&](int i){
                            ++v;
                        },
                        [&](){
                            ++c;
                        }));
                while(v != values * 3 || c != 3);
                s0.unsubscribe();
                s1.unsubscribe();
                s2.unsubscribe();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "range observe_on : " << n << " subscribed, " << v << " on_next calls, " << msElapsed.count() << "ms elapsed, " << v / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

                if (--runs > 0) {
                    self();
                }
            };

            w.schedule(loop);
        }
    }
}

SCENARIO("subscription traits", "[subscription][traits]"){
    GIVEN("given some subscription types"){
        auto es = rx::make_subscription();
        rx::composite_subscription cs;
        WHEN("tested"){
            THEN("is_subscription value is true for empty subscription"){
                REQUIRE(rx::is_subscription<decltype(es)>::value);
            }
            THEN("is_subscription value is true for composite_subscription"){
                REQUIRE(rx::is_subscription<decltype(cs)>::value);
            }
        }
    }
}

SCENARIO("non-subscription traits", "[subscription][traits]"){
    GIVEN("given some non-subscription types"){
        auto l = [](){};
        int i = 0;
        void* v = nullptr;
        WHEN("tested"){
            THEN("is_subscription value is false for lambda"){
                REQUIRE(!rx::is_subscription<decltype(l)>::value);
            }
            THEN("is_subscription value is false for int"){
                REQUIRE(!rx::is_subscription<decltype(i)>::value);
            }
            THEN("is_subscription value is false for void*"){
                REQUIRE(!rx::is_subscription<decltype(v)>::value);
            }
            THEN("is_subscription value is false for void"){
                REQUIRE(!rx::is_subscription<void>::value);
            }
        }
    }
}

SCENARIO("subscription static", "[subscription]"){
    GIVEN("given a subscription"){
        int i=0;
        auto s = rx::make_subscription([&i](){++i;});
        WHEN("not used"){
            THEN("is subscribed"){
                REQUIRE(s.is_subscribed());
            }
            THEN("i is 0"){
                REQUIRE(i == 0);
            }
        }
        WHEN("used"){
            THEN("is not subscribed when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("is not subscribed when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("i is 1 when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(i == 1);
            }
            THEN("i is 1 when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(i == 1);
            }
        }
    }
}

SCENARIO("subscription empty", "[subscription]"){
    GIVEN("given an empty subscription"){
        auto s = rx::make_subscription();
        WHEN("not used"){
            THEN("is not subscribed"){
                REQUIRE(!s.is_subscribed());
            }
        }
        WHEN("used"){
            THEN("is not subscribed when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("is not subscribed when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
        }
    }
}

SCENARIO("subscription composite", "[subscription]"){
    GIVEN("given a subscription"){
        int i=0;
        rx::composite_subscription s;
        s.add(rx::make_subscription());
        s.add(rx::make_subscription([&i](){++i;}));
        s.add([&i](){++i;});
        WHEN("not used"){
            THEN("is subscribed"){
                REQUIRE(s.is_subscribed());
            }
            THEN("i is 0"){
                REQUIRE(i == 0);
            }
        }
        WHEN("used"){
            THEN("is not subscribed when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("is not subscribed when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("i is 2 when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(i == 2);
            }
            THEN("i is 2 when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(i == 2);
            }
        }
    }
}

