// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_SCOPE_HPP)
#define RXCPP_SOURCES_RX_SCOPE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class ResourceFactory, class ObservableFactory>
struct scope_traits
{
    typedef typename std::decay<ResourceFactory>::type resource_factory_type;
    typedef typename std::decay<ObservableFactory>::type observable_factory_type;
    typedef decltype((*(resource_factory_type*)nullptr)()) resource_type;
    typedef decltype((*(observable_factory_type*)nullptr)(resource_type())) collection_type;
    typedef typename collection_type::value_type value_type;

    static_assert(is_subscription<resource_type>::value, "ResourceFactory must return a subscription");
};

template<class ResourceFactory, class ObservableFactory>
struct scope : public source_base<typename scope_traits<ResourceFactory, ObservableFactory>::value_type>
{
    typedef scope_traits<ResourceFactory, ObservableFactory> traits;
    typedef typename traits::resource_factory_type resource_factory_type;
    typedef typename traits::observable_factory_type observable_factory_type;
    typedef typename traits::resource_type resource_type;
    typedef typename traits::value_type value_type;

    struct values
    {
        values(resource_factory_type rf, observable_factory_type of)
            : resource_factory(std::move(rf))
            , observable_factory(std::move(of))
        {
        }
        resource_factory_type resource_factory;
        observable_factory_type observable_factory;
    };
    values initial;


    scope(resource_factory_type rf, observable_factory_type of)
        : initial(std::move(rf), std::move(of))
    {
    }

    template<class Subscriber>
    void on_subscribe(Subscriber o) const {

        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(values i, Subscriber o)
                : values(i)
                , out(std::move(o))
            {
                out.add(lifetime);
            }
            Subscriber out;
            rxu::detail::maybe<resource_type> resource;
            composite_subscription lifetime;
        };

        auto state = std::shared_ptr<state_type>(new state_type(initial, std::move(o)));

        state->resource = on_exception(
            [&](){return state->resource_factory(); },
            state->out);
        if (state->resource.empty()) {
            return;
        }

        auto selectedCollection = on_exception(
            [state](){return state->observable_factory(state->resource.get()); },
            state->out);
        if (selectedCollection.empty()) {
            return;
        }

        selectedCollection->subscribe(make_subscriber<value_type>(
            state->lifetime,
            // on_next
            [state](value_type st) {
                state->out.on_next(st);
            },
            // on_error
            [state](std::exception_ptr e) {
                state->out.on_error(e);
                state->resource->unsubscribe();
            },
            // on_completed
            [state]() {
                state->out.on_completed();
                state->resource->unsubscribe();
            }
        ));
    }
};

}

template<class ResourceFactory, class ObservableFactory>
auto scope(ResourceFactory rf, ObservableFactory of)
    ->      observable<typename detail::scope_traits<ResourceFactory, ObservableFactory>::value_type, detail::scope<ResourceFactory, ObservableFactory>> {
    return  observable<typename detail::scope_traits<ResourceFactory, ObservableFactory>::value_type, detail::scope<ResourceFactory, ObservableFactory>>(
                                                                                                      detail::scope<ResourceFactory, ObservableFactory>(std::move(rf), std::move(of)));
}

}

}

#endif
