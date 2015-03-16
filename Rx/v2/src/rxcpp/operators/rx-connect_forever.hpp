// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_CONNECT_FOREVER_HPP)
#define RXCPP_OPERATORS_RX_CONNECT_FOREVER_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class ConnectableObservable>
struct connect_forever : public operator_base<T>
{
    typedef rxu::decay_t<ConnectableObservable> source_type;

    source_type source;

    explicit connect_forever(source_type o)
        : source(std::move(o))
    {
        source.connect();
    }

    template<class Subscriber>
    void on_subscribe(Subscriber&& o) const {
        source.subscribe(std::forward<Subscriber>(o));
    }
};

class connect_forever_factory
{
public:
    connect_forever_factory() {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   connect_forever<rxu::value_type_t<rxu::decay_t<Observable>>, Observable>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   connect_forever<rxu::value_type_t<rxu::decay_t<Observable>>, Observable>>(
                                                                          connect_forever<rxu::value_type_t<rxu::decay_t<Observable>>, Observable>(std::forward<Observable>(source)));
    }
};

}

inline auto connect_forever()
    ->      detail::connect_forever_factory {
    return  detail::connect_forever_factory();
}

}

}

#endif
