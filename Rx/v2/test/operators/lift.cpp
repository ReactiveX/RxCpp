
#define RXCPP_USE_OBSERVABLE_MEMBERS 1

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

        static rx::subscriber<value_type, this_type> make(const dest_type& d, const test_type& t) {
            return rx::make_subscriber<value_type>(d, this_type(d, t));
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = w.start<int>(
                [&xs, &invoked]() {
                    return xs
                        .lift(liftfilter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        }))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 400)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = w.start<int>(
                [&xs, &invoked]() {
                    return xs
                        >> liftfilter([&invoked](int x) {
                            invoked++;
                            return IsPrime(x);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        >> rxo::as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 400)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        long invoked = 0;

        record messages[] = {
            on_next(110, 1),
            on_next(180, 2),
            on_next(230, 3),
            on_next(270, 4),
            on_next(340, 5),
            on_next(380, 6),
            on_next(390, 7),
            on_next(450, 8),
            on_next(470, 9),
            on_next(560, 10),
            on_next(580, 11),
            on_completed(600)
        };
        auto xs = sc.make_hot_observable(rxu::to_vector(messages));

        WHEN("filtered to ints that are primes"){

            auto res = w.start<int>(
                [&xs, &invoked]() {
                    auto predicate = [&](int x){
                        invoked++;
                        return IsPrime(x);
                    };
                    return xs
                        .lift([=](rx::subscriber<int> dest){
                            return rx::make_subscriber<int>(
                                dest,
                                [=](int n){
                                    bool pass = false;
                                    try{pass = predicate(n);} catch(...){dest.on_error(std::current_exception());};
                                    if (pass) {dest.on_next(n);}
                                },
                                [=](std::exception_ptr e){dest.on_error(e);},
                                [=](){dest.on_completed();});
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                record items[] = {
                    on_next(230, 3),
                    on_next(340, 5),
                    on_next(390, 7)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                life items[] = {
                    subscribe(200, 400)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

