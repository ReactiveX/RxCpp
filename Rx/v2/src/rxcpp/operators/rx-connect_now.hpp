// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_CONNECT_NOW_HPP)
#define RXCPP_OPERATORS_RX_CONNECT_NOW_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class ConnectableObservable>
struct connect_now : public operator_base<T>
{
    typedef typename std::decay<ConnectableObservable>::type source_type;

    source_type source;

    explicit connect_now(source_type o)
        : source(std::move(o))
    {
        source.connect();
    }

    template<class Subscriber>
    void on_subscribe(Subscriber&& o) {
        source.subscribe(std::forward<Subscriber>(o));
    }
};

class connect_now_factory
{
public:
    connect_now_factory() {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   connect_now<typename std::decay<Observable>::type::value_type, Observable>> {
        return  observable<typename std::decay<Observable>::type::value_type,   connect_now<typename std::decay<Observable>::type::value_type, Observable>>(
                                                                                connect_now<typename std::decay<Observable>::type::value_type, Observable>(std::forward<Observable>(source)));
    }
};

}

inline auto connect_now()
    ->      detail::connect_now_factory {
    return  detail::connect_now_factory();
}

}

}

#endif
