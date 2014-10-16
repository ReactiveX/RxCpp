// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_NEVER_HPP)
#define RXCPP_SOURCES_RX_NEVER_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class T>
struct never : public source_base<T>
{
    template<class Subscriber>
    void on_subscribe(Subscriber) const {
    }
};

}

template<class T>
auto never()
    ->      observable<T, detail::never<T>> {
    return  observable<T, detail::never<T>>(detail::never<T>());
}

}

}

#endif
