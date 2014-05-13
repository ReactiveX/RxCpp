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
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Predicate>::type test_type;
    source_type source;
    test_type test;

    template<class CT, class CP>
    static auto check(int) -> decltype((*(CP*)nullptr)(*(CT*)nullptr));
    template<class CT, class CP>
    static void check(...);
    filter(source_type o, test_type p)
        : source(std::move(o))
        , test(std::move(p))
    {
        static_assert(std::is_convertible<decltype(check<T, test_type>(0)), bool>::value, "filter Predicate must be a function with the signature bool(T)");
    }
    template<class Subscriber>
    void on_subscribe(const Subscriber& o) {
        source.subscribe(
            o,
        // on_next
            [this, o](T t) {
                bool filtered = false;
                try {
                   filtered = !this->test(t);
                } catch(...) {
                    o.on_error(std::current_exception());
                    return;
                }
                if (!filtered) {
                    o.on_next(t);
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
        );
    }
};

template<class Predicate>
class filter_factory
{
    typedef typename std::decay<Predicate>::type test_type;
    test_type predicate;
public:
    filter_factory(test_type p) : predicate(std::move(p)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   filter<typename std::decay<Observable>::type::value_type, Observable, Predicate>> {
        return  observable<typename std::decay<Observable>::type::value_type,   filter<typename std::decay<Observable>::type::value_type, Observable, Predicate>>(
                                                                                filter<typename std::decay<Observable>::type::value_type, Observable, Predicate>(std::forward<Observable>(source), std::move(predicate)));
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
