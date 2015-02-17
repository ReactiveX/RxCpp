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
    typedef rxu::decay_t<ObservableFactory> observable_factory_type;
    typedef decltype((*(observable_factory_type*)nullptr)()) collection_type;
    typedef typename collection_type::value_type value_type;
};

template<class ObservableFactory>
struct defer : public source_base<rxu::value_type_t<defer_traits<ObservableFactory>>>
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
    ->      observable<rxu::value_type_t<detail::defer_traits<ObservableFactory>>,    detail::defer<ObservableFactory>> {
    return  observable<rxu::value_type_t<detail::defer_traits<ObservableFactory>>,    detail::defer<ObservableFactory>>(
                                                                                      detail::defer<ObservableFactory>(std::move(of)));
}

}

}

#endif
