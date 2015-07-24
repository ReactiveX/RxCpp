#define RXCPP_SUBJECT_TEST_ASYNC 1

#include "../test.h"

#include <future>


const int static_onnextcalls = 10000000;
static int aliased = 0;

SCENARIO("for loop locks mutex", "[hide][for][mutex][long][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop"){
        WHEN("locking mutex 100 million times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int c = 0;
            int n = 1;
            auto start = clock::now();
            std::mutex m;
            for (int i = 0; i < onnextcalls; i++) {
                std::unique_lock<std::mutex> guard(m);
                ++c;
            }
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "loop mutex          : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

namespace syncwithvoid {
template<class T, class OnNext>
class sync_subscriber
{
public:
    OnNext onnext;
    bool issubscribed;
    explicit sync_subscriber(OnNext on)
        : onnext(on)
        , issubscribed(true)
    {
    }
    bool is_subscribed() {return issubscribed;}
    void unsubscribe() {issubscribed = false;}
    void on_next(T v) {
        onnext(v);
    }
};
}
SCENARIO("for loop calls void on_next(int)", "[hide][for][asyncobserver][baseline][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop"){
        WHEN("calling on_next 100 million times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto c = std::addressof(aliased);
            *c = 0;
            int n = 1;
            auto start = clock::now();
            auto onnext = [c](int){++*c;};
            syncwithvoid::sync_subscriber<int, decltype(onnext)> scbr(onnext);
            for (int i = 0; i < onnextcalls && scbr.is_subscribed(); i++) {
                scbr.on_next(i);
            }
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "loop void           : " << n << " subscribed, " << *c << " on_next calls, " << msElapsed.count() << "ms elapsed " << *c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

namespace asyncwithready {
// ready is an immutable class.
class ready
{
public:
    typedef std::function<void()> onthen_type;
private:
    std::function<void(onthen_type)> setthen;
public:
    ready() {}
    ready(std::function<void(onthen_type)> st) : setthen(st) {}
    bool is_ready() {return !setthen;}
    void then(onthen_type ot) {
        if (is_ready()) {
            abort();
        }
        setthen(ot);
    }
};
template<class T, class OnNext>
class async_subscriber
{
public:
    OnNext onnext;
    bool issubscribed;
    int count;
    explicit async_subscriber(OnNext on)
        : onnext(on)
        , issubscribed(true)
        , count(0)
    {
    }
    bool is_subscribed() {return issubscribed;}
    void unsubscribe() {issubscribed = false;}
    ready on_next(T v) {
        // push v onto queue

        // under some condition pop v off of queue and pass it on
        onnext(v);

        // for demo purposes
        // simulate queue full every 100000 items
        if (count == 100000) {
            // 'queue is full'
            ready no([this](ready::onthen_type ot){
                // full version will sync producer and consumer (in producer push and consumer pop)
                // and decide when to restart the producer
                if (!this->count) {
                    ot();
                }
            });
            // set queue empty since the demo has no separate consumer thread
            count = 0;
            // 'queue is empty'
            return no;
        }
        static const ready yes;
        return yes;
    }
};
}
SCENARIO("for loop calls ready on_next(int)", "[hide][for][asyncobserver][ready][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop"){
        WHEN("calling on_next 100 million times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto c = std::addressof(aliased);
            *c = 0;
            int n = 1;
            auto start = clock::now();
            auto onnext = [&c](int){++*c;};
            asyncwithready::async_subscriber<int, decltype(onnext)> scbr(onnext);
            asyncwithready::ready::onthen_type chunk;
            int i = 0;
            chunk = [&chunk, scbr, i, onnextcalls]() mutable {
                for (; i < onnextcalls && scbr.is_subscribed(); i++) {
                    auto controller = scbr.on_next(i);
                    if (!controller.is_ready()) {
                        controller.then(chunk);
                        return;
                    }
                }
            };
            chunk();
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "loop ready          : " << n << " subscribed, " << *c << " on_next calls, " << msElapsed.count() << "ms elapsed " << *c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

namespace asyncwithfuture {
class unit {};
template<class T, class OnNext>
class async_subscriber
{
public:
    OnNext onnext;
    bool issubscribed;
    explicit async_subscriber(OnNext on)
        : onnext(on)
        , issubscribed(true)
    {
    }
    bool is_subscribed() {return issubscribed;}
    void unsubscribe() {issubscribed = false;}
    std::future<unit> on_next(T v) {
        std::promise<unit> ready;
        ready.set_value(unit());
        onnext(v); return ready.get_future();}
};
}
SCENARIO("for loop calls std::future<unit> on_next(int)", "[hide][for][asyncobserver][future][long][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop"){
        WHEN("calling on_next 100 million times"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto c = std::addressof(aliased);
            *c = 0;
            int n = 1;
            auto start = clock::now();
            auto onnext = [&c](int){++*c;};
            asyncwithfuture::async_subscriber<int, decltype(onnext)> scbr(onnext);
            for (int i = 0; i < onnextcalls && scbr.is_subscribed(); i++) {
                auto isready = scbr.on_next(i);
                if (isready.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout) {
                    isready.wait();
                }
            }
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "loop future<unit>   : " << n << " subscribed, " << *c << " on_next calls, " << msElapsed.count() << "ms elapsed " << *c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

SCENARIO("for loop calls observer", "[hide][for][observer][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop"){
        WHEN("observing 100 million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int& c = aliased;
            int n = 1;

            c = 0;
            auto start = clock::now();
            auto o = rx::make_observer<int>(
                [&c](int){++c;},
                [](std::exception_ptr){abort();});
            for (int i = 0; i < onnextcalls; i++) {
                o.on_next(i);
            }
            o.on_completed();
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "loop -> observer    : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec"<< std::endl;
        }
    }
}

SCENARIO("for loop calls subscriber", "[hide][for][subscriber][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop"){
        WHEN("observing 100 million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int& c = aliased;
            int n = 1;

            c = 0;
            auto start = clock::now();
            auto o = rx::make_subscriber<int>(
                [&c](int){++c;},
                [](std::exception_ptr){abort();});
            for (int i = 0; i < onnextcalls && o.is_subscribed(); i++) {
                o.on_next(i);
            }
            o.on_completed();
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "loop -> subscriber  : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("range calls subscriber", "[hide][range][subscriber][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a range"){
        WHEN("observing 100 million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int& c = aliased;
            int n = 1;

            c = 0;
            auto start = clock::now();

            rxs::range<int>(1, onnextcalls).subscribe(
                [&c](int){
                    ++c;
                },
                [](std::exception_ptr){abort();});

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "range -> subscriber : " << n << " subscribed, " << c << " on_next calls, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("for loop calls subject", "[hide][for][subject][subjects][long][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a for loop and a subject"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;

            for (int n = 0; n < 10; n++)
            {
                auto p = std::make_shared<int>(0);
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                auto o = sub.get_subscriber();

                o.add(rx::make_subscription([c, n, onnextcalls](){
                    auto expected = n * onnextcalls;
                    REQUIRE(*c == expected);
                }));

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, o]() {
                        auto source = sub.get_observable();
                        while(o.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            rx::composite_subscription cs;
                            source.subscribe(
                                rx::make_subscriber<int>(
                                cs,
                                [cs](int){
                                    cs.unsubscribe();
                                },
                                [](std::exception_ptr){abort();}));
                        }
                        return 0;
                    });
#endif
                    sub.get_observable().subscribe(
                        [c, p](int){
                            ++(*c);
                        },
                        [](std::exception_ptr){abort();});
                }

                auto start = clock::now();
                for (int i = 0; i < onnextcalls && o.is_subscribed(); i++) {
#if RXCPP_DEBUG_SUBJECT_RACE
                    if (*p != *c) abort();
                    (*p) += n;
#endif
                    o.on_next(i);
                }
                o.on_completed();
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "loop -> subject     : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << *c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
            }
        }
    }
}

SCENARIO("range calls subject", "[hide][range][subject][subjects][long][perf]"){
    const int& onnextcalls = static_onnextcalls;
    GIVEN("a range and a subject"){
        WHEN("multicasting a million ints"){
            using namespace std::chrono;
            typedef steady_clock clock;
            for (int n = 0; n < 10; n++)
            {
                auto p = std::make_shared<int>(0);
                auto c = std::make_shared<int>(0);
                rxsub::subject<int> sub;

#if RXCPP_SUBJECT_TEST_ASYNC
                std::vector<std::future<int>> f(n);
#endif

                auto o = sub.get_subscriber();

                o.add(rx::make_subscription([c, n, onnextcalls](){
                    auto expected = n * onnextcalls;
                    REQUIRE(*c == expected);
                }));

                for (int i = 0; i < n; i++) {
#if RXCPP_SUBJECT_TEST_ASYNC
                    f[i] = std::async([sub, o]() {
                        while(o.is_subscribed()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            rx::composite_subscription cs;
                            sub.get_observable().subscribe(cs, [cs](int){
                                cs.unsubscribe();
                            },
                            [](std::exception_ptr){abort();});
                        }
                        return 0;
                    });
#endif
                    sub.get_observable()
                        .subscribe(
                            [c, p](int){
                               ++(*c);
                            },
                            [](std::exception_ptr){abort();}
                        );
                }

                auto start = clock::now();
                rxs::range<int>(1, onnextcalls)
#if RXCPP_DEBUG_SUBJECT_RACE
                    .filter([c, p, n](int){
                        if (*p != *c) abort();
                        (*p) += n;
                        return true;
                    })
#endif
                    .subscribe(o);
                auto finish = clock::now();
                auto msElapsed = duration_cast<milliseconds>(finish-start);
                std::cout << "range -> subject    : " << n << " subscribed, " << (*c) << " on_next calls, " << msElapsed.count() << "ms elapsed " << *c / (msElapsed.count() / 1000.0) << " ops/sec"<< std::endl;
            }
        }
    }
}


SCENARIO("subject - infinite source", "[subject][subjects]"){
    GIVEN("a subject and an infinite source"){

        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 1),
            on.next(110, 2),
            on.next(220, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(410, 6),
            on.next(520, 7),
            on.next(630, 8),
            on.next(710, 9),
            on.next(870, 10),
            on.next(940, 11),
            on.next(1020, 12)
        });

        rxsub::subject<int> s;

        auto results1 = w.make_subscriber<int>();

        auto results2 = w.make_subscriber<int>();

        auto results3 = w.make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            w.schedule_absolute(100, [&s, &o](const rxsc::schedulable&){
                s = rxsub::subject<int>(); o = s.get_subscriber();});
            w.schedule_absolute(200, [&xs, &o](const rxsc::schedulable&){
                xs.subscribe(o);});
            w.schedule_absolute(1000, [&o](const rxsc::schedulable&){
                o.unsubscribe();});

            w.schedule_absolute(300, [&s, &results1](const rxsc::schedulable&){
                s.get_observable().subscribe(results1);});
            w.schedule_absolute(400, [&s, &results2](const rxsc::schedulable&){
                s.get_observable().subscribe(results2);});
            w.schedule_absolute(900, [&s, &results3](const rxsc::schedulable&){
                s.get_observable().subscribe(results3);});

            w.schedule_absolute(600, [&results1](const rxsc::schedulable&){
                results1.unsubscribe();});
            w.schedule_absolute(700, [&results2](const rxsc::schedulable&){
                results2.unsubscribe();});
            w.schedule_absolute(800, [&results1](const rxsc::schedulable&){
                results1.unsubscribe();});
            w.schedule_absolute(950, [&results3](const rxsc::schedulable&){
                results3.unsubscribe();});

            w.start();

            THEN("result1 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(340, 5),
                    on.next(410, 6),
                    on.next(520, 7)
                });
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(410, 6),
                    on.next(520, 7),
                    on.next(630, 8)
                });
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(940, 11)
                });
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("subject - finite source", "[subject][subjects]"){
    GIVEN("a subject and an finite source"){

        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(70, 1),
            on.next(110, 2),
            on.next(220, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(410, 6),
            on.next(520, 7),
            on.completed(630),
            on.next(640, 9),
            on.completed(650),
            on.error(660, std::runtime_error("error on unsubscribed stream"))
        });

        rxsub::subject<int> s;

        auto results1 = w.make_subscriber<int>();

        auto results2 = w.make_subscriber<int>();

        auto results3 = w.make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            w.schedule_absolute(100, [&s, &o](const rxsc::schedulable&){
                s = rxsub::subject<int>(); o = s.get_subscriber();});
            w.schedule_absolute(200, [&xs, &o](const rxsc::schedulable&){
                xs.subscribe(o);});
            w.schedule_absolute(1000, [&o](const rxsc::schedulable&){
                o.unsubscribe();});

            w.schedule_absolute(300, [&s, &results1](const rxsc::schedulable&){
                s.get_observable().subscribe(results1);});
            w.schedule_absolute(400, [&s, &results2](const rxsc::schedulable&){
                s.get_observable().subscribe(results2);});
            w.schedule_absolute(900, [&s, &results3](const rxsc::schedulable&){
                s.get_observable().subscribe(results3);});

            w.schedule_absolute(600, [&results1](const rxsc::schedulable&){
                results1.unsubscribe();});
            w.schedule_absolute(700, [&results2](const rxsc::schedulable&){
                results2.unsubscribe();});
            w.schedule_absolute(800, [&results1](const rxsc::schedulable&){
                results1.unsubscribe();});
            w.schedule_absolute(950, [&results3](const rxsc::schedulable&){
                results3.unsubscribe();});

            w.start();

            THEN("result1 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(340, 5),
                    on.next(410, 6),
                    on.next(520, 7)
                });
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(410, 6),
                    on.next(520, 7),
                    on.completed(630)
                });
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                auto required = rxu::to_vector({
                    on.completed(900)
                });
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}


SCENARIO("subject - on_error in source", "[subject][subjects]"){
    GIVEN("a subject and a source with an error"){

        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("subject on_error in stream");

        auto xs = sc.make_hot_observable({
            on.next(70, 1),
            on.next(110, 2),
            on.next(220, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(410, 6),
            on.next(520, 7),
            on.error(630, ex),
            on.next(640, 9),
            on.completed(650),
            on.error(660, std::runtime_error("error on unsubscribed stream"))
        });

        rxsub::subject<int> s;

        auto results1 = w.make_subscriber<int>();

        auto results2 = w.make_subscriber<int>();

        auto results3 = w.make_subscriber<int>();

        WHEN("multicasting an infinite source"){

            auto o = s.get_subscriber();

            w.schedule_absolute(100, [&s, &o](const rxsc::schedulable&){
                s = rxsub::subject<int>(); o = s.get_subscriber();});
            w.schedule_absolute(200, [&xs, &o](const rxsc::schedulable&){
                xs.subscribe(o);});
            w.schedule_absolute(1000, [&o](const rxsc::schedulable&){
                o.unsubscribe();});

            w.schedule_absolute(300, [&s, &results1](const rxsc::schedulable&){
                s.get_observable().subscribe(results1);});
            w.schedule_absolute(400, [&s, &results2](const rxsc::schedulable&){
                s.get_observable().subscribe(results2);});
            w.schedule_absolute(900, [&s, &results3](const rxsc::schedulable&){
                s.get_observable().subscribe(results3);});

            w.schedule_absolute(600, [&results1](const rxsc::schedulable&){
                results1.unsubscribe();});
            w.schedule_absolute(700, [&results2](const rxsc::schedulable&){
                results2.unsubscribe();});
            w.schedule_absolute(800, [&results1](const rxsc::schedulable&){
                results1.unsubscribe();});
            w.schedule_absolute(950, [&results3](const rxsc::schedulable&){
                results3.unsubscribe();});

            w.start();

            THEN("result1 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(340, 5),
                    on.next(410, 6),
                    on.next(520, 7)
                });
                auto actual = results1.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result2 contains expected messages"){
                auto required = rxu::to_vector({
                    on.next(410, 6),
                    on.next(520, 7),
                    on.error(630, ex)
                });
                auto actual = results2.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("result3 contains expected messages"){
                auto required = rxu::to_vector({
                    on.error(900, ex)
                });
                auto actual = results3.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}
