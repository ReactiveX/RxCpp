// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_ALL_HPP)
#define RXCPP_OPERATORS_RX_ALL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Predicate>
struct all
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Predicate> test_type;
    test_type test;

    all(test_type t)
        : test(std::move(t))
    {
    }

    template<class Subscriber>
    struct all_observer
    {
        typedef all_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        test_type test;
        mutable bool done;

        all_observer(dest_type d, test_type t)
            : dest(std::move(d))
            , test(std::move(t)),
              done(false)
        {
        }
        void on_next(source_value_type v) const {
            auto filtered = on_exception([&]() {
                return !this->test(v); },
                dest);
            if (filtered.empty()) {
                return;
            }
            if (filtered.get() && !done) {
                done = true;
                dest.on_next(false);
                dest.on_completed();
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            if(!done) {
                done = true;
                dest.on_next(true);
                dest.on_completed();
            }
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, test_type t) {
            return make_subscriber<value_type>(d, this_type(d, std::move(t)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(all_observer<Subscriber>::make(std::move(dest), test)) {
        return      all_observer<Subscriber>::make(std::move(dest), test);
    }
};

template <class Predicate>
class all_factory
{
    typedef rxu::decay_t<Predicate> test_type;

    test_type test;
public:
    all_factory(test_type t) : test(t) { }

    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(all<rxu::value_type_t<rxu::decay_t<Observable>>, Predicate>(test))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(all<rxu::value_type_t<rxu::decay_t<Observable>>, Predicate>(test));
    }
};

}

template <class Predicate>
inline auto all(Predicate test)
->      detail::all_factory<Predicate> {
return  detail::all_factory<Predicate>(test);
}

}

}

#endif
