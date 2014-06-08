// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_DEFER_HPP)
#define RXCPP_SOURCES_RX_DEFER_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class ObservableFactory>
struct defer_traits
{
    typedef typename std::decay<ObservableFactory>::type observable_factory_type;
    typedef decltype((*(observable_factory_type*)nullptr)()) collection_type;
    typedef typename collection_type::value_type value_type;
};

template<class ObservableFactory>
struct defer : public source_base<typename defer_traits<ObservableFactory>::value_type>
{
    typedef defer<ObservableFactory> this_type;
    typedef defer_traits<ObservableFactory> traits;

    typedef typename traits::observable_factory_type observable_factory_type;
    typedef typename traits::collection_type collection_type;

    observable_factory_type observable_factory;

    defer(observable_factory_type of)
        : observable_factory(std::move(of))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {

        auto selectedCollection = on_exception(
            [this](){return this->observable_factory();},
            o);
        if (selectedCollection.empty()) {
            return;
        }

        selectedCollection->subscribe(o);
    }
};

}

template<class ObservableFactory>
auto defer(ObservableFactory of)
    ->      observable<typename detail::defer_traits<ObservableFactory>::value_type,    detail::defer<ObservableFactory>> {
    return  observable<typename detail::defer_traits<ObservableFactory>::value_type,    detail::defer<ObservableFactory>>(
                                                                                        detail::defer<ObservableFactory>(std::move(of)));
}

}

}

#endif
