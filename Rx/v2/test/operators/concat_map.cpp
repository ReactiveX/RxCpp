#include "../test.h"

static const int static_tripletCount = 100;

SCENARIO("concat_map pythagorian ranges", "[hide][range][concat_map][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("some ranges"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto sc = rxsc::make_immediate();
            //auto sc = rxsc::make_current_thread();
            auto so = rx::identity_one_worker(sc);

            int c = 0;
            int ct = 0;
            int n = 1;
            auto start = clock::now();
            auto triples =
                rxs::range(1, so)
                    .concat_map(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .concat_map(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){++c; return x*x + y*y == z*z;})
                                            .map([z, x](int y){return std::make_tuple(x, y, z);})
                                            // forget type to workaround lambda deduction bug on msvc 2013
                                            .as_dynamic();},
                                    [](int /*x*/, std::tuple<int,int,int> triplet){return triplet;})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int /*z*/, std::tuple<int,int,int> triplet){return triplet;});
            triples
                .take(tripletCount)
                .subscribe(
                    rxu::apply_to([&ct](int /*x*/,int /*y*/,int /*z*/){++ct;}),
                    [](std::exception_ptr){abort();});
            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "concat pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;

        }
    }
}

SCENARIO("synchronize concat_map pythagorian ranges", "[hide][range][concat_map][synchronize][pythagorian][perf]"){
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
                    .concat_map(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .concat_map(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){
                                                ++c;
                                                if (x*x + y*y == z*z) {
                                                    return true;}
                                                else {
                                                    return false;}})
                                            .map([z, x](int y){return std::make_tuple(x, y, z);})
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
            std::cout << "concat sync pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("observe_on concat_map pythagorian ranges", "[hide][range][concat_map][observe_on][pythagorian][perf]"){
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
                    .concat_map(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .concat_map(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){
                                                ++c;
                                                if (x*x + y*y == z*z) {
                                                    return true;}
                                                else {
                                                    return false;}})
                                            .map([z, x](int y){return std::make_tuple(x, y, z);})
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
            std::cout << "concat observe_on pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("serialize concat_map pythagorian ranges", "[hide][range][concat_map][serialize][pythagorian][perf]"){
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
                    .concat_map(
                        [&c, so](int z){
                            return rxs::range(1, z, 1, so)
                                .concat_map(
                                    [&c, so, z](int x){
                                        return rxs::range(x, z, 1, so)
                                            .filter([&c, z, x](int y){
                                                ++c;
                                                if (x*x + y*y == z*z) {
                                                    return true;}
                                                else {
                                                    return false;}})
                                            .map([z, x](int y){return std::make_tuple(x, y, z);})
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
            std::cout << "concat serial pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("concat_map completes", "[concat_map][map][operators]"){
    GIVEN("two cold observables. one of ints. one of strings."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> i_on;
        const rxsc::test::messages<std::string> s_on;

        auto xs = sc.make_cold_observable({
            i_on.next(100, 4),
            i_on.next(200, 2),
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
                        .concat_map(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(500, "qux"),
                    s_on.next(600, "foo"),
                    s_on.next(650, "bar"),
                    s_on.next(700, "baz"),
                    s_on.next(750, "qux"),
                    s_on.completed(800)
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

            THEN("there were 2 subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(550, 800)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }

        WHEN("streamed each int is mapped to the strings"){

            auto res = w.start(
                [&]() {
                    return xs >>
                        rxo::concat_map(
                            [&](int){
                                return ys;},
                            [](int, std::string s){
                                return s;},
                            rx::identity_current_thread()) >>
                        // forget type to workaround lambda deduction bug on msvc 2013
                        rxo::as_dynamic();
                }
            );

            THEN("the output contains strings repeated for each int"){
                auto required = rxu::to_vector({
                    s_on.next(350, "foo"),
                    s_on.next(400, "bar"),
                    s_on.next(450, "baz"),
                    s_on.next(500, "qux"),
                    s_on.next(600, "foo"),
                    s_on.next(650, "bar"),
                    s_on.next(700, "baz"),
                    s_on.next(750, "qux"),
                    s_on.completed(800)
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

            THEN("there were 2 subscription and unsubscription to the strings"){
                auto required = rxu::to_vector({
                    s_on.subscribe(300, 550),
                    s_on.subscribe(550, 800)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

