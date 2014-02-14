// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_FILTER_HPP)
#define RXCPP_OPERATORS_RX_FILTER_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Predicate>
struct filter : public operator_base<T>
{
    Observable source;
    Predicate test;

    template<class CT, class CP>
    static auto check(int) -> decltype((*(CP*)nullptr)(*(CT*)nullptr));
    template<class CT, class CP>
    static void check(...);
    filter(Observable o, Predicate p)
        : source(std::move(o))
        , test(std::move(p))
    {
        static_assert(std::is_convertible<decltype(check<T, Predicate>(0)), bool>::value, "filter Predicate must be a function with the signature bool(T)");
    }
    template<class I>
    void on_subscribe(observer<T, I> o) {
        o.add(source.subscribe(
            o.get_subscription(),
        // on_next
            [this, o](T t) {
                bool filtered = false;
                try {
                   filtered = !this->test(t);
                } catch(...) {
                    o.on_error(std::current_exception());
                    o.unsubscribe();
                }
                if (!filtered) {
                    o.on_next(std::move(t));
                }
            },
        // on_error
            [o](std::exception_ptr e) {
                o.on_error(e);
            },
        // on_completed
            [o]() {
                o.on_completed();
            }
        ));
    }
};

template<class Predicate>
class filter_factory
{
    Predicate predicate;
public:
    filter_factory(Predicate p) : predicate(std::move(p)) {}
    template<class Observable>
    auto operator()(Observable source)
        ->      observable<typename Observable::value_type, filter<typename Observable::value_type, Observable, Predicate>> {
        return  observable<typename Observable::value_type, filter<typename Observable::value_type, Observable, Predicate>>(
                                                            filter<typename Observable::value_type, Observable, Predicate>(source, std::move(predicate)));
    }
};

}

template<class Predicate>
auto filter(Predicate p)
    ->      detail::filter_factory<Predicate> {
    return  detail::filter_factory<Predicate>(std::move(p));
}

}

}

#endif
