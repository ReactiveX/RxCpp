// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_DEFER_HPP)
#define RXCPP_SOURCES_RX_DEFER_HPP

#include "../rx-includes.hpp"

/*! \file rx-defer.hpp

    \brief Returns an observable that calls the specified observable factory to create an observable for each new observer that subscribes.

    \tparam ObservableFactory  the type of the observable factory

    \param  of  the observable factory function to invoke for each observer that subscribes to the resulting observable

    \return  observable whose observers' subscriptions trigger an invocation of the given observable factory function

    \sample
    \snippet defer.cpp defer sample
    \snippet output.txt defer sample
*/

namespace rxcpp {

namespace sources {

namespace detail {

template<class ObservableFactory>
struct defer_traits
{
    using observable_factory_type = rxu::decay_t<ObservableFactory>;
    using collection_type = decltype(std::declval<observable_factory_type>()());
    using value_type = typename collection_type::value_type;
};

template<class ObservableFactory>
struct defer : public source_base<rxu::value_type_t<defer_traits<ObservableFactory>>>
{
    using this_type = defer<ObservableFactory>;
    using traits = defer_traits<ObservableFactory>;

    using observable_factory_type = typename traits::observable_factory_type;
    using collection_type = typename traits::collection_type;

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

/*! @copydoc rx-defer.hpp
 */
template<class ObservableFactory>
auto defer(ObservableFactory of)
    ->      observable<rxu::value_type_t<detail::defer_traits<ObservableFactory>>,    detail::defer<ObservableFactory>> {
    return  observable<rxu::value_type_t<detail::defer_traits<ObservableFactory>>,    detail::defer<ObservableFactory>>(
                                                                                      detail::defer<ObservableFactory>(std::move(of)));
}

}

}

#endif
