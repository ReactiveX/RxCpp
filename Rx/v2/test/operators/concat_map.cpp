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
                                    [](int x, std::tuple<int,int,int> triplet){return triplet;})
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int z, std::tuple<int,int,int> triplet){return triplet;});
            triples
                .take(tripletCount)
                .subscribe(
                    rxu::apply_to([&ct](int x,int y,int z){++ct;}),
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

            std::mutex lock;
            std::condition_variable wake;

            auto sc = rxsc::make_event_loop();
            //auto sc = rxsc::make_new_thread();
            auto so = rx::synchronize_in_one_worker(sc);

            int c = 0;
            std::atomic<int> ct(0);
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
                                    [](int x, std::tuple<int,int,int> triplet){return triplet;},
                                    so)
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int z, std::tuple<int,int,int> triplet){return triplet;},
                        so);
            triples
                .take(tripletCount)
                .subscribe(
                    rxu::apply_to([&ct](int x,int y,int z){
                        ++ct;}),
                    [](std::exception_ptr){abort();},
                    [&](){
                        wake.notify_one();});

            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&](){return ct == tripletCount;});

            auto finish = clock::now();
            auto msElapsed = duration_cast<milliseconds>(finish.time_since_epoch()) -
                   duration_cast<milliseconds>(start.time_since_epoch());
            std::cout << "concat sync pythagorian range : " << n << " subscribed, " << c << " filtered to, " << ct << " triplets, " << msElapsed.count() << "ms elapsed " << c / (msElapsed.count() / 1000.0) << " ops/sec" << std::endl;
        }
    }
}

SCENARIO("serialize concat_map pythagorian ranges", "[hide][range][concat_map][serialize][pythagorian][perf]"){
    const int& tripletCount = static_tripletCount;
    GIVEN("some ranges"){
        WHEN("generating pythagorian triplets"){
            using namespace std::chrono;
            typedef steady_clock clock;

            std::mutex lock;
            std::condition_variable wake;

            auto sc = rxsc::make_event_loop();
            //auto sc = rxsc::make_new_thread();
            auto so = rx::serialize_one_worker(sc);

            int c = 0;
            std::atomic<int> ct(0);
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
                                    [](int x, std::tuple<int,int,int> triplet){return triplet;},
                                    so)
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();},
                        [](int z, std::tuple<int,int,int> triplet){return triplet;},
                        so);
            triples
                .take(tripletCount)
                .subscribe(
                    rxu::apply_to([&ct](int x,int y,int z){
                        ++ct;}),
                    [](std::exception_ptr){abort();},
                    [&](){
                        wake.notify_one();});

            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&](){return ct == tripletCount;});

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
        typedef rxsc::test::messages<int> m;
        typedef rxsc::test::messages<std::string> ms;
        typedef rxn::subscription life;

        typedef m::recorded_type irecord;
        auto ion_next = m::on_next;
        auto ion_completed = m::on_completed;
        auto isubscribe = m::subscribe;

        typedef ms::recorded_type srecord;
        auto son_next = ms::on_next;
        auto son_completed = ms::on_completed;
        auto ssubscribe = ms::subscribe;

        irecord int_messages[] = {
            ion_next(100, 4),
            ion_next(200, 2),
            ion_completed(500)
        };
        auto xs = sc.make_cold_observable(int_messages);

        srecord string_messages[] = {
            son_next(50, "foo"),
            son_next(100, "bar"),
            son_next(150, "baz"),
            son_next(200, "qux"),
            son_completed(250)
        };
        auto ys = sc.make_cold_observable(string_messages);

        WHEN("each int is mapped to the strings"){

            auto res = w.start<std::string>(
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
                srecord items[] = {
                    son_next(350, "foo"),
                    son_next(400, "bar"),
                    son_next(450, "baz"),
                    son_next(500, "qux"),
                    son_next(600, "foo"),
                    son_next(650, "bar"),
                    son_next(700, "baz"),
                    son_next(750, "qux"),
                    son_completed(800)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ints"){
                life items[] = {
                    isubscribe(200, 700)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there were 2 subscription and unsubscription to the strings"){
                life items[] = {
                    ssubscribe(300, 550),
                    ssubscribe(550, 800)
                };
                auto required = rxu::to_vector(items);
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

