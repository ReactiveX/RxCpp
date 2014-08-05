// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_USING_HPP)
#define RXCPP_SOURCES_RX_USING_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class ResourceFactory, class ObservableFactory>
struct using_traits
{
    typedef typename std::decay<ResourceFactory>::type resource_factory_type;
    typedef typename std::decay<ObservableFactory>::type observable_factory_type;
    typedef decltype((*(observable_factory_type*)nullptr)()) collection_type;
    typedef typename collection_type::value_type value_type;
};

template<class ResourceFactory, class ObservableFactory>
struct _using : public source_base<typename using_traits<ResourceFactory, ObservableFactory>::value_type>
{
    typedef using_traits<ResourceFactory, ObservableFactory> traits;
    typedef typename traits::resource_factory_type resource_factory_type;
    typedef typename traits::observable_factory_type observable_factory_type;

    resource_factory_type resource_factory;
    observable_factory_type observable_factory;

    _using(resource_factory_type rf, observable_factory_type of)
        : resource_factory(std::move(rf))
        , observable_factory(std::move(of))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        auto selectedResource = on_exception(
            [&](){return this->resource_factory(); },
            o);
        if (selectedResource.empty()) {
            return;
        }

        auto selectedCollection = on_exception(
            [&](){return this->observable_factory(selectedResource.get()); },
            o);
        if (selectedCollection.empty()) {
            return;
        }

        selectedCollection->subscribe(o);
    }
};

}

template<class ResourceFactory, class ObservableFactory>
auto _using(ResourceFactory rf, ObservableFactory of)
    ->      observable<typename detail::using_traits<ResourceFactory, ObservableFactory>::value_type, detail::_using<ResourceFactory, ObservableFactory>> {
    return  observable<typename detail::using_traits<ResourceFactory, ObservableFactory>::value_type, detail::_using<ResourceFactory, ObservableFactory>>(
                                                                                                      detail::_using<ResourceFactory, ObservableFactory>(std::move(rf), std::move(of)));
}

}

}

#endif
