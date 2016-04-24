// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_START_WITH_HPP)
#define RXCPP_OPERATORS_RX_START_WITH_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class StartObservable>
class start_with_factory
{
public:
    using start_type = rxu::decay_t<StartObservable>;
    
    start_type start;
    
    explicit start_with_factory(start_type s) : start(s) {}

    template<class Observable>
    auto operator()(Observable source)
        -> decltype(start.concat(source)) {
        return      start.concat(source);
    }
};
    
}

template<class Value0, class... ValueN>
auto start_with(Value0 v0, ValueN... vn)
    ->     detail::start_with_factory<decltype(rxs::from(rxu::decay_t<Value0>(v0), rxu::decay_t<Value0>(vn)...))> {
    return detail::start_with_factory<decltype(rxs::from(rxu::decay_t<Value0>(v0), rxu::decay_t<Value0>(vn)...))>(
                                               rxs::from(rxu::decay_t<Value0>(v0), rxu::decay_t<Value0>(vn)...));
}

}

}

#endif
