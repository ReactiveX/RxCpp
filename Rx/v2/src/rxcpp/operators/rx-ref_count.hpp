// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_REF_COUNT_HPP)
#define RXCPP_OPERATORS_RX_REF_COUNT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class ConnectableObservable>
struct ref_count : public operator_base<T>
{
    typedef typename std::decay<ConnectableObservable>::type source_type;

    source_type source;
    long subscribers;
    composite_subscription connection;

    explicit ref_count(source_type o)
        : source(std::move(o))
        , subscribers(0)
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber&& o) {
        auto needConnect = ++subscribers == 1;
        o.add(make_subscription(
            [this](){
                if (--this->subscribers == 0) {
                    this->connection.unsubscribe();
                }
            }));
        source.subscribe(std::forward<Subscriber>(o));
        if (needConnect) {
            connection = source.connect();
        }
    }
};

class ref_count_factory
{
public:
    ref_count_factory() {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   ref_count<typename std::decay<Observable>::type::value_type, Observable>> {
        return  observable<typename std::decay<Observable>::type::value_type,   ref_count<typename std::decay<Observable>::type::value_type, Observable>>(
                                                                                ref_count<typename std::decay<Observable>::type::value_type, Observable>(std::forward<Observable>(source)));
    }
};

}

inline auto ref_count()
    ->      detail::ref_count_factory {
    return  detail::ref_count_factory();
}

}

}

#endif
