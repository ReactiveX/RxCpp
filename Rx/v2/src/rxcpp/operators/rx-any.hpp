// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_ANY_HPP)
#define RXCPP_OPERATORS_RX_ANY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Predicate>
struct any
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Predicate> test_type;
    test_type test;

    any(test_type t)
        : test(std::move(t))
    {
    }

    template<class Subscriber>
    struct any_observer
    {
        typedef any_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        test_type test;
        mutable bool done;

        any_observer(dest_type d, test_type t)
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
            if (!filtered.get() && !done) {
                done = true;
                dest.on_next(true);
                dest.on_completed();
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            if(!done) {
                done = true;
                dest.on_next(false);
                dest.on_completed();
            }
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, test_type t) {
            return make_subscriber<value_type>(d, this_type(d, std::move(t)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(any_observer<Subscriber>::make(std::move(dest), test)) {
        return      any_observer<Subscriber>::make(std::move(dest), test);
    }
};

template <class Predicate>
class any_factory
{
    typedef rxu::decay_t<Predicate> test_type;

    test_type test;
public:
    any_factory(test_type t) : test(t) { }

    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(any<rxu::value_type_t<rxu::decay_t<Observable>>, Predicate>(test))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(any<rxu::value_type_t<rxu::decay_t<Observable>>, Predicate>(test));
    }
};

}

template <class Predicate>
inline auto exists(Predicate test)
->      detail::any_factory<Predicate> {
    return  detail::any_factory<Predicate>(test);
}

template <class T>
inline auto contains(T value)
->      detail::any_factory<std::function<bool(T)>> {
    return  detail::any_factory<std::function<bool(T)>>([value](T n) { return n == value; });
}
}

}

#endif
