// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_CREATE_HPP)
#define RXCPP_SOURCES_RX_CREATE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class T, class OnSubscribe>
struct create : public source_base<T>
{
    typedef create<T, OnSubscribe> this_type;

    typedef rxu::decay_t<OnSubscribe> on_subscribe_type;

    on_subscribe_type on_subscribe_function;

    create(on_subscribe_type os)
        : on_subscribe_function(std::move(os))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber o) const {

        on_exception(
            [&](){
                this->on_subscribe_function(o);
                return true;
            },
            o);
    }
};

}

template<class T, class OnSubscribe>
auto create(OnSubscribe os)
    ->      observable<T,   detail::create<T, OnSubscribe>> {
    return  observable<T,   detail::create<T, OnSubscribe>>(
                            detail::create<T, OnSubscribe>(std::move(os)));
}

}

}

#endif
