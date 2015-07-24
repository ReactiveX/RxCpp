#include "../test.h"

namespace detail {

template<class Predicate>
struct liftfilter
{
    typedef typename std::decay<Predicate>::type test_type;
    test_type test;

    liftfilter(test_type t)
        : test(t)
    {
    }

    template<class Subscriber>
    struct filter_observer : public rx::observer_base<typename std::decay<Subscriber>::type::value_type>
    {
        typedef filter_observer<Subscriber> this_type;
        typedef rx::observer_base<typename std::decay<Subscriber>::type::value_type> base_type;
        typedef typename base_type::value_type value_type;
        typedef typename std::decay<Subscriber>::type dest_type;
        typedef rx::observer<value_type, this_type> observer_type;
        dest_type dest;
        test_type test;

        filter_observer(dest_type d, test_type t)
            : dest(d)
            , test(t)
        {
        }
        void on_next(typename dest_type::value_type v) const {
            bool filtered = false;
            try {
               filtered = !test(v);
            } catch(...) {
                dest.on_error(std::current_exception());
                return;
            }
            if (!filtered) {
                dest.on_next(v);
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static rx::subscriber<value_type, observer_type> make(const dest_type& d, const test_type& t) {
            return rx::make_subscriber<value_type>(d, observer_type(this_type(d, t)));
        }
    };

    template<class Subscriber>
    auto operator()(const Subscriber& dest) const
        -> decltype(filter_observer<Subscriber>::make(dest, test)) {
        return      filter_observer<Subscriber>::make(dest, test);
    }
};

}

namespace {

template<class Predicate>
auto liftfilter(Predicate&& p)
    ->      detail::liftfilter<typename std::decay<Predicate>::type> {
    return  detail::liftfilter<typename std::decay<Predicate>::type>(std::forward<Predicate>(p));
}

bool IsPrime(int x)
{
    if (x < 2) return false;
    for (int i = 2; i <= x/2; ++i)
    {
        if (x % i == 0)
            return false;
    }
    return true;
}

}

SCENARIO("lift liftfilter stops on disposal", "[where][filter][lift][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600)
        });

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [&xs, &invoked]() {
                    return xs
                        .lift<int>(liftfilter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        }))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

SCENARIO("stream lift liftfilter stops on disposal", "[where][filter][lift][stream][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600)
        });

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [&xs, &invoked]() {
                    return xs
                        >> rxo::lift<int>(liftfilter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        }))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

SCENARIO("lift lambda filter stops on disposal", "[where][filter][lift][lambda][operators]"){
    GIVEN("a test hot observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        auto xs = sc.make_hot_observable({
            on.next(110, 1),
            on.next(180, 2),
            on.next(230, 3),
            on.next(270, 4),
            on.next(340, 5),
            on.next(380, 6),
            on.next(390, 7),
            on.next(450, 8),
            on.next(470, 9),
            on.next(560, 10),
            on.next(580, 11),
            on.completed(600)
        });

        WHEN("filtered to ints that are primes"){

            auto res = w.start(
                [&xs, &invoked]() {
                    auto predicate = [&](int x){
                        invoked++;
                        return IsPrime(x);
                    };
                    return xs
                        .lift<int>([=](rx::subscriber<int> dest){
                            // VS2013 deduction issue requires dynamic (type-forgetting)
                            return rx::make_subscriber<int>(
                                dest,
                                rx::make_observer_dynamic<int>(
                                    [=](int n){
                                        bool pass = false;
                                        try{pass = predicate(n);} catch(...){dest.on_error(std::current_exception());};
                                        if (pass) {dest.on_next(n);}
                                    },
                                    [=](std::exception_ptr e){dest.on_error(e);},
                                    [=](){dest.on_completed();}));
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                auto required = rxu::to_vector({
                    on.next(230, 3),
                    on.next(340, 5),
                    on.next(390, 7)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

