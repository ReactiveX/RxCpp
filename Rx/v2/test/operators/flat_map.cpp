#include "../test.h"
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-filter.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-take.hpp>
#include <rxcpp/operators/rx-flat_map.hpp>
#include <rxcpp/operators/rx-observe_on.hpp>

static const int static_tripletCount = 100;

SCENARIO("pythagorian for loops", "[hide][for][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("a for loop"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            int c = 0;
            int ct = 0;
            int n = 1;
            auto start = clock::now();
            for(int z = 1;; ++z)
            {
                for(int x = 1; x <= z; ++x)
                {
                    for(int y = x; y <= z; ++y)
                    {
                        ++c;
                        if(x*x + y*y == z*z)
                        {
                            ++ct;
                            if(ct == tripletCount)
                                goto done;
                        }
                    }
                }
            }
            done:
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "pythagorian for   : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

SCENARIO("merge_transform pythagorian ranges", "[hide][range][merge_transform][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("some ranges"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::identity_immediate();

            int c = 0;
            int ct = 0;
            int n = 1;
            auto start = clock::now();
            auto triples =
                rxs::range(1, so)
                    .merge_transform(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .flat_map(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){++c; return x*x + y*y == z*z;})
                                            .transform([z, x](int y){return std::make_tuple(x, y, z);})
                                            // forget type to workaround lambda deduction bug on msvc 2013
                                            .as_dynamic();},
                                    [](int /*x*/, std::tuple<int,int,int> triplet){return triplet;})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int /*z*/, std::tuple<int,int,int> triplet){return triplet;});
            triples
                .take(tripletCount)
                .subscribe(
                    rxu::apply_to([&ct](int /*x*/,int /*y*/,int /*z*/){
                        ++ct;
                    }));
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

SCENARIO("synchronize merge_transform pythagorian ranges", "[hide][range][merge_transform][synchronize][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("some ranges"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::synchronize_event_loop();

            int c = 0;
            int n = 1;
            auto start = clock::now();
            auto triples =
                rxs::range(1, so)
                    .merge_transform(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .merge_transform(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){
                                                ++c;
                                                if (x*x + y*y == z*z) {
                                                    return true;}
                                                else {
                                                    return false;}})
                                            .transform([z, x](int y){return std::make_tuple(x, y, z);})
                                            // forget type to workaround lambda deduction bug on msvc 2013
                                            .as_dynamic();},
                                    [](int /*x*/, std::tuple<int,int,int> triplet){return triplet;},
                                    so)
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int /*z*/, std::tuple<int,int,int> triplet){return triplet;},
                        so);
            int ct = triples
                .take(tripletCount)
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge sync pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("observe_on merge_transform pythagorian ranges", "[hide][range][merge_transform][observe_on][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("some ranges"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::observe_on_event_loop();

            int c = 0;
            int n = 1;
            auto start = clock::now();
            auto triples =
                rxs::range(1, so)
                    .merge_transform(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .merge_transform(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){
                                                ++c;
                                                if (x*x + y*y == z*z) {
                                                    return true;}
                                                else {
                                                    return false;}})
                                            .transform([z, x](int y){return std::make_tuple(x, y, z);})
                                            // forget type to workaround lambda deduction bug on msvc 2013
                                            .as_dynamic();},
                                    [](int /*x*/, std::tuple<int,int,int> triplet){return triplet;},
                                    so)
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int /*z*/, std::tuple<int,int,int> triplet){return triplet;},
                        so);
            int ct = triples
                .take(tripletCount)
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge observe_on pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("serialize merge_transform pythagorian ranges", "[hide][range][merge_transform][serialize][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("some ranges"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto so = rx::serialize_event_loop();

            int c = 0;
            int n = 1;
            auto start = clock::now();
            auto triples =
                rxs::range(1, so)
                    .merge_transform(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .merge_transform(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){
                                                ++c;
                                                if (x*x + y*y == z*z) {
                                                    return true;}
                                                else {
                                                    return false;}})
                                            .transform([z, x](int y){return std::make_tuple(x, y, z);})
                                            // forget type to workaround lambda deduction bug on msvc 2013
                                            .as_dynamic();},
                                    [](int /*x*/, std::tuple<int,int,int> triplet){return triplet;},
                                    so)
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int /*z*/, std::tuple<int,int,int> triplet){return triplet;},
                        so);
            int ct = triples
                .take(tripletCount)
                .as_blocking()
                .count();

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "merge serial pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("flat_map completes", "[flat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
            i_on.next(300, 3),
            i_on.next(400, 1),
            i_on.completed(500)
        });

        auto ys = sc.make_cold_observable({
            s_on.next(50, "foo"),
            s_on.next(100, "bar"),
            s_on.next(150, "baz"),
            s_on.next(200, "qux"),
            s_on.completed(250)
        });

        WHEN("each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::flat_map(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(450, "foo"),
                    s_on.next(500, "qux"),
                    s_on.next(500, "bar"),
                    s_on.next(550, "baz"),
                    s_on.next(550, "foo"),
                    s_on.next(600, "qux"),
                    s_on.next(600, "bar"),
                    s_on.next(650, "baz"),
                    s_on.next(650, "foo"),
                    s_on.next(700, "qux"),
                    s_on.next(700, "bar"),
                    s_on.next(750, "baz"),
                    s_on.next(800, "qux"),
                    s_on.completed(850)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 700)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(400, 650),
                    s_on.subscribe(500, 750),
                    s_on.subscribe(600, 850)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("merge_transform completes", "[merge_transform][transform][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
            i_on.next(300, 3),
            i_on.next(400, 1),
            i_on.completed(500)
        });

        auto ys = sc.make_cold_observable({
            s_on.next(50, "foo"),
            s_on.next(100, "bar"),
            s_on.next(150, "baz"),
            s_on.next(200, "qux"),
            s_on.completed(250)
        });

        WHEN("each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        | rxo::merge_transform(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        | rxo::as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(450, "foo"),
                    s_on.next(500, "qux"),
                    s_on.next(500, "bar"),
                    s_on.next(550, "baz"),
                    s_on.next(550, "foo"),
                    s_on.next(600, "qux"),
                    s_on.next(600, "bar"),
                    s_on.next(650, "baz"),
                    s_on.next(650, "foo"),
                    s_on.next(700, "qux"),
                    s_on.next(700, "bar"),
                    s_on.next(750, "baz"),
                    s_on.next(800, "qux"),
                    s_on.completed(850)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 700)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(400, 650),
                    s_on.subscribe(500, 750),
                    s_on.subscribe(600, 850)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }

        WHEN("each int is mapped to the strings with coordinator"){

            auto res = w.start(
                [&]() {
                    return xs
                        .merge_transform(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;},
                            rx::identity_current_thread())
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(450, "foo"),
                    s_on.next(500, "qux"),
                    s_on.next(500, "bar"),
                    s_on.next(550, "baz"),
                    s_on.next(550, "foo"),
                    s_on.next(600, "qux"),
                    s_on.next(600, "bar"),
                    s_on.next(650, "baz"),
                    s_on.next(650, "foo"),
                    s_on.next(700, "qux"),
                    s_on.next(700, "bar"),
                    s_on.next(750, "baz"),
                    s_on.next(800, "qux"),
                    s_on.completed(850)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 700)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(400, 650),
                    s_on.subscribe(500, 750),
                    s_on.subscribe(600, 850)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("merge_transform source never ends", "[merge_transform][transform][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
            i_on.next(300, 3),
            i_on.next(400, 1),
            i_on.next(500, 5),
            i_on.next(700, 0)
        });

        auto ys = sc.make_cold_observable({
            s_on.next(50, "foo"),
            s_on.next(100, "bar"),
            s_on.next(150, "baz"),
            s_on.next(200, "qux"),
            s_on.completed(250)
        });

        WHEN("each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        .merge_transform([&](int){return ys;}, [](int, std::string s){return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(450, "foo"),
                    s_on.next(500, "qux"),
                    s_on.next(500, "bar"),
                    s_on.next(550, "baz"),
                    s_on.next(550, "foo"),
                    s_on.next(600, "qux"),
                    s_on.next(600, "bar"),
                    s_on.next(650, "baz"),
                    s_on.next(650, "foo"),
                    s_on.next(700, "qux"),
                    s_on.next(700, "bar"),
                    s_on.next(750, "baz"),
                    s_on.next(750, "foo"),
                    s_on.next(800, "qux"),
                    s_on.next(800, "bar"),
                    s_on.next(850, "baz"),
                    s_on.next(900, "qux"),
                    s_on.next(950, "foo")
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 1000)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(400, 650),
                    s_on.subscribe(500, 750),
                    s_on.subscribe(600, 850),
                    s_on.subscribe(700, 950),
                    s_on.subscribe(900, 1000)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("merge_transform inner error", "[merge_transform][transform][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
            i_on.next(300, 3),
            i_on.next(400, 1),
            i_on.completed(500)
        });

        std::runtime_error ex("filter on_error from inner source");

        auto ys = sc.make_cold_observable({
            s_on.next(55, "foo"),
            s_on.next(104, "bar"),
            s_on.next(153, "baz"),
            s_on.next(202, "qux"),
            s_on.error(301, ex)
        });

        WHEN("each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        .merge_transform([&](int){return ys;}, [](int, std::string s){return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(355, "foo"),
                    s_on.next(404, "bar"),
                    s_on.next(453, "baz"),
                    s_on.next(455, "foo"),
                    s_on.next(502, "qux"),
                    s_on.next(504, "bar"),
                    s_on.next(553, "baz"),
                    s_on.next(555, "foo"),
                    s_on.error(601, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 601)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 601),
                    s_on.subscribe(400, 601),
                    s_on.subscribe(500, 601),
                    s_on.subscribe(600, 601)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("merge_transform, no result selector, no coordination", "[merge_transform][transform][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
            i_on.next(300, 3),
            i_on.next(400, 1),
            i_on.completed(500)
        });

        auto ys = sc.make_cold_observable({
            s_on.next(50, "foo"),
            s_on.next(100, "bar"),
            s_on.next(150, "baz"),
            s_on.next(200, "qux"),
            s_on.completed(250)
        });

        WHEN("each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        .merge_transform(
                            [&](int){
                                return ys;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(450, "foo"),
                    s_on.next(500, "qux"),
                    s_on.next(500, "bar"),
                    s_on.next(550, "baz"),
                    s_on.next(550, "foo"),
                    s_on.next(600, "qux"),
                    s_on.next(600, "bar"),
                    s_on.next(650, "baz"),
                    s_on.next(650, "foo"),
                    s_on.next(700, "qux"),
                    s_on.next(700, "bar"),
                    s_on.next(750, "baz"),
                    s_on.next(800, "qux"),
                    s_on.completed(850)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 700)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(400, 650),
                    s_on.subscribe(500, 750),
                    s_on.subscribe(600, 850)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("merge_transform, no result selector, with coordination", "[merge_transform][transform][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
            i_on.next(300, 3),
            i_on.next(400, 1),
            i_on.completed(500)
        });

        auto ys = sc.make_cold_observable({
            s_on.next(50, "foo"),
            s_on.next(100, "bar"),
            s_on.next(150, "baz"),
            s_on.next(200, "qux"),
            s_on.completed(250)
        });

        WHEN("each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs
                        .merge_transform(
                            [&](int){
                                return ys;},
                            rx::identity_current_thread())
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(450, "foo"),
                    s_on.next(500, "qux"),
                    s_on.next(500, "bar"),
                    s_on.next(550, "baz"),
                    s_on.next(550, "foo"),
                    s_on.next(600, "qux"),
                    s_on.next(600, "bar"),
                    s_on.next(650, "baz"),
                    s_on.next(650, "foo"),
                    s_on.next(700, "qux"),
                    s_on.next(700, "bar"),
                    s_on.next(750, "baz"),
                    s_on.next(800, "qux"),
                    s_on.completed(850)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                auto required = rxu::to_vector({
                    i_on.subscribe(200, 700)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were four subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(400, 650),
                    s_on.subscribe(500, 750),
                    s_on.subscribe(600, 850)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
