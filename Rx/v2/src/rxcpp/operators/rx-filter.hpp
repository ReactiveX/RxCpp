// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_FILTER_HPP)
#define RXCPP_OPERATORS_RX_FILTER_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Predicate>
struct filter
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Predicate> test_type;
    test_type test;

    filter(test_type t)
        : test(std::move(t))
    {
    }

    template<class Subscriber>
    struct filter_observer
    {
        typedef filter_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        test_type test;

        filter_observer(dest_type d, test_type t)
            : dest(std::move(d))
            , test(std::move(t))
        {
        }
        void on_next(source_value_type v) const {
            auto filtered = on_exception([&](){
                return !this->test(v);},
                dest);
            if (filtered.empty()) {
                return;
            }
            if (!filtered.get()) {
                dest.on_next(v);
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<value_type, observer<value_type, this_type>> make(dest_type d, test_type t) {
            return make_subscriber<value_type>(d, this_type(d, std::move(t)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(filter_observer<Subscriber>::make(std::move(dest), test)) {
        return      filter_observer<Subscriber>::make(std::move(dest), test);
    }
};

template<class Predicate>
class filter_factory
{
    typedef rxu::decay_t<Predicate> test_type;
    test_type predicate;
public:
    filter_factory(test_type p) : predicate(std::move(p)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(filter<rxu::value_type_t<rxu::decay_t<Observable>>, test_type>(predicate))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(filter<rxu::value_type_t<rxu::decay_t<Observable>>, test_type>(predicate));
    }
};

}

template<class Predicate>
auto filter(Predicate&& p)
    ->      detail::filter_factory<Predicate> {
    return  detail::filter_factory<Predicate>(std::forward<Predicate>(p));
}

}

}

#endif
