#include "../test.h"
#include <rxcpp/operators/rx-concat.hpp>
#include <rxcpp/operators/rx-group_by.hpp>
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-merge.hpp>
#include <rxcpp/operators/rx-take.hpp>
#include <rxcpp/operators/rx-start_with.hpp>
#include <rxcpp/operators/rx-observe_on.hpp>

#include <locale>

SCENARIO("range partitioned by group_by across hardware threads to derive pi", "[hide][pi][group_by][observe_on][long][perf]"){
    GIVEN("a for loop"){
        WHEN("partitioning pi series across all hardware threads"){

            std::atomic_int c;
            c = 0;
            auto pi = [&](int k) {
                ++c;
                return ( k % 2 == 0 ? -4.0L : 4.0L ) / ( ( 2.0L * k ) - 1.0L );
            };

            using namespace std::chrono;
            auto start = steady_clock::now();

            // share an output thread across all the producer threads
            auto outputthread = rxcpp::observe_on_one_worker(rxcpp::observe_on_new_thread().create_coordinator().get_scheduler());

            struct work
            {
                int index;
                int first;
                int last;
            };

            // use all available hardware threads
            auto total = rxcpp::observable<>::range(0, (2 * std::thread::hardware_concurrency()) - 1).
                map(
                    [](int index){
                        static const int chunk = 100000000 / (2 * std::thread::hardware_concurrency());
                        int first = (chunk * index) + 1;
                        int last =   chunk * (index + 1);
                        return work{index, first, last};}
                    ).
                group_by(
                    [](work w) -> int {return w.index % std::thread::hardware_concurrency();},
                    [](work w){return w;}).
                map(
                    [=](rxcpp::grouped_observable<int, work> onproc) {
                        auto key = onproc.get_key();
                        // share a producer thread across all the ranges in this group of chunks
                        auto producerthread = rxcpp::observe_on_one_worker(rxcpp::observe_on_new_thread().create_coordinator().get_scheduler());
                        return onproc.
                            map(
                                [=](work w){
                                    std::stringstream message;
                                    message << std::setw(3) << w.index << ": range - " << w.first << "-" << w.last;

                                    return rxcpp::observable<>::range(w.first, w.last, producerthread).
                                        map(pi).
                                        sum(). // each thread maps and reduces its contribution to the answer
                                        map(
                                            [=](long double v){
                                                std::stringstream message;
                                                message << key << " on " << std::this_thread::get_id() << " - value: " << std::setprecision(16) << v;
                                                return std::make_tuple(message.str(), v);
                                            }).
                                        start_with(std::make_tuple(message.str(), 0.0L)).
                                        as_dynamic();
                                }).
                            concat(). // only subscribe to one range at a time in this group.
                            observe_on(outputthread).
                            map(rxcpp::util::apply_to(
                                [](std::string message, long double v){
                                    std::cout << message << std::endl;
                                    return v;
                                })).
                            as_dynamic();
                    }).
                merge().
                sum(). // reduces the contributions from all the threads to the answer
                as_blocking().
                last();

            std::cout << std::setprecision(16) << "Pi: " << total << std::endl;
            auto finish = steady_clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "pi using group_by and concat to partition the work : " << c << " calls to pi(), " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

SCENARIO("range partitioned by dividing work across hardware threads to derive pi", "[hide][pi][observe_on][long][perf]"){
    GIVEN("a for loop"){
        WHEN("partitioning pi series across all hardware threads"){

            std::atomic_int c;
            c = 0;
            auto pi = [&](int k) {
                ++c;
                return ( k % 2 == 0 ? -4.0L : 4.0L ) / ( ( 2.0L * k ) - 1.0L );
            };

            using namespace std::chrono;
            auto start = steady_clock::now();

            struct work
            {
                int index;
                int first;
                int last;
            };

            // use all available hardware threads
            auto total = rxcpp::observable<>::range(0, (2 * std::thread::hardware_concurrency()) - 1).
                map(
                    [](int index){
                        static const int chunk = 100000000 / (2 * std::thread::hardware_concurrency());
                        int first = (chunk * index) + 1;
                        int last =   chunk * (index + 1);
                        return work{index, first, last};
                    }).
                map(
                    [=](work w){
                        std::stringstream message;
                        message << std::setw(3) << w.index << ": range - " << w.first << "-" << w.last;

                        // create a new thread for every chunk
                        return rxcpp::observable<>::range(w.first, w.last, rxcpp::observe_on_new_thread()).
                            map(pi).
                            sum(). // each thread maps and reduces its contribution to the answer
                            map(
                                [=](long double v){
                                    std::stringstream message;
                                    message << w.index << " on " << std::this_thread::get_id() << " - value: " << std::setprecision(16) << v;
                                    return std::make_tuple(message.str(), v);
                                }).
                            start_with(std::make_tuple(message.str(), 0.0L)).
                            as_dynamic();
                    }).
                merge(rxcpp::observe_on_new_thread()).
                map(rxcpp::util::apply_to(
                    [](std::string message, long double v){
                        std::cout << message << std::endl;
                        return v;
                    })).
                sum(). // reduces the contributions from all the threads to the answer
                as_blocking().
                last();

            std::cout << std::setprecision(16) << "Pi: " << total << std::endl;
            auto finish = steady_clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish-start);
            std::cout << "pi using division of the whole range to partition the work : " << c << " calls to pi(), " << msElapsed.count() << "ms elapsed, " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

char whitespace(char c) {
    return std::isspace<char>(c, std::locale::classic());
}

std::string trim(std::string s) {
    auto first = std::find_if_not(s.begin(), s.end(), whitespace);
    auto last = std::find_if_not(s.rbegin(), s.rend(), whitespace);
    if (last != s.rend()) {
        s.erase(s.end() - (last-s.rbegin()), s.end());
    }
    s.erase(s.begin(), first);
    return s;
}

bool tolowerLess(char lhs, char rhs) {
    return std::tolower(lhs, std::locale::classic()) < std::tolower(rhs, std::locale::classic());
}

bool tolowerStringLess(const std::string& lhs, const std::string& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), tolowerLess);
}

SCENARIO("group_by", "[group_by][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<std::string> on;
        int keyInvoked = 0;
        int marbleInvoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(90, "error"),
            on.next(110, "error"),
            on.next(130, "error"),
            on.next(220, "  foo"),
            on.next(240, " FoO "),
            on.next(270, "baR  "),
            on.next(310, "foO "),
            on.next(350, " Baz   "),
            on.next(360, "  qux "),
            on.next(390, "   bar"),
            on.next(420, " BAR  "),
            on.next(470, "FOO "),
            on.next(480, "baz  "),
            on.next(510, " bAZ "),
            on.next(530, "    fOo    "),
            on.completed(570),
            on.next(580, "error"),
            on.completed(600),
            on.error(650, std::runtime_error("error in completed sequence"))
        });

        WHEN("group normalized strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        .group_by(
                            [&](std::string v){
                                ++keyInvoked;
                                return trim(std::move(v));
                            },
                            [&](std::string v){
                                ++marbleInvoked;
                                std::reverse(v.begin(), v.end());
                                return v;
                            },
                            tolowerStringLess)
                        .map([](const rxcpp::grouped_observable<std::string, std::string>& g){return g.get_key();})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of group keys"){
                auto required = rxu::to_vector({
                    on.next(220, "foo"),
                    on.next(270, "baR"),
                    on.next(350, "Baz"),
                    on.next(360, "qux"),
                    on.completed(570)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 570)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("key selector was invoked for each value"){
                REQUIRE(12 == keyInvoked);
            }

            THEN("marble selector was invoked for each value"){
                REQUIRE(12 == marbleInvoked);
            }
        }
    }
}

SCENARIO("group_by take 1", "[group_by][take][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<long> on;
        int keyInvoked = 0;
        int marbleInvoked = 0;
        int groupEmitted = 0;

        auto xs = sc.make_hot_observable({
            on.next(130, -1),
            on.next(220, 0),
            on.next(240, -1),
            on.next(270, 2),
            on.next(310, -3),
            on.next(350, 4),
            on.next(360, -5),
            on.next(390, 6),
            on.next(420, -7),
            on.next(470, 8),
            on.next(480, -9),
            on.completed(570)
        });

        WHEN("1 group of ints is emitted"){

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::group_by(
                            [&](long v) {
                                ++keyInvoked;
                                return v % 2;
                            },
                            [&](long v){
                                ++marbleInvoked;
                                return v;
                            })
                        | rxo::take(1)
                        | rxo::map([&](const rxcpp::grouped_observable<long, long>& g) -> rxcpp::observable<long> {
                            ++groupEmitted;
                            return g;
                        })
                        | rxo::merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    on.next(220, 0),
                    on.next(270, 2),
                    on.next(350, 4),
                    on.next(390, 6),
                    on.next(470, 8),
                    on.completed(570)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 570)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("key selector was invoked for each value"){
                REQUIRE(10 == keyInvoked);
            }

            THEN("marble selector was invoked for each value"){
                REQUIRE(5 == marbleInvoked);
            }

            THEN("1 group emitted"){
                REQUIRE(1 == groupEmitted);
            }
        }
    }
}

SCENARIO("group_by take 1 take 4", "[group_by][take][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<long> on;
        int keyInvoked = 0;
        int marbleInvoked = 0;
        int groupEmitted = 0;

        auto xs = sc.make_hot_observable({
            on.next(130, -1),
            on.next(220, 0),
            on.next(240, -1),
            on.next(270, 2),
            on.next(310, -3),
            on.next(350, 4),
            on.next(360, -5),
            on.next(390, 6),
            on.next(420, -7),
        });

        WHEN("1 group of ints is emitted"){

            auto res = w.start(
                [&]() {
                    return xs
                        .group_by(
                            [&](long v) {
                                ++keyInvoked;
                                return v % 2;
                            },
                            [&](long v){
                                ++marbleInvoked;
                                return v;
                            })
                        .take(1)
                        .map([&](const rxcpp::grouped_observable<long, long>& g) -> rxcpp::observable<long> {
                            ++groupEmitted;
                            return g.take(4);
                        })
                        .merge()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    on.next(220, 0),
                    on.next(270, 2),
                    on.next(350, 4),
                    on.next(390, 6),
                    on.completed(390)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 390)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("key selector was invoked for each value"){
                REQUIRE(7 == keyInvoked);
            }

            THEN("marble selector was invoked for each value"){
                REQUIRE(4 == marbleInvoked);
            }

            THEN("1 group emitted"){
                REQUIRE(1 == groupEmitted);
            }
        }
    }
}