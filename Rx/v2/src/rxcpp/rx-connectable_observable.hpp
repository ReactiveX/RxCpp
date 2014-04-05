// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_CONNECTABLE_OBSERVABLE_HPP)
#define RXCPP_RX_CONNECTABLE_OBSERVABLE_HPP

#include "rx-includes.hpp"

namespace rxcpp {

template<class T, class SourceOperator>
class connectable_observable
    : public observable<T, SourceOperator>
{
public:
    typedef connectable_observable<T, SourceOperator> this_type;
    typedef tag_connectable_observable observable_tag;
    typedef observable<T, SourceOperator> base_type;

    connectable_observable()
    {
    }

    explicit connectable_observable(const SourceOperator& o)
        : base_type(o)
    {
    }
    explicit connectable_observable(SourceOperator&& o)
        : base_type(std::move(o))
    {
    }

    // implicit conversion between observables of the same value_type
    template<class SO>
    connectable_observable(const connectable_observable<T, SO>& o)
        : base_type(o)
    {}
    // implicit conversion between observables of the same value_type
    template<class SO>
    connectable_observable(connectable_observable<T, SO>&& o)
        : base_type(std::move(o))
    {}

    composite_subscription connect(composite_subscription cs = composite_subscription()) {
        base_type::source_operator.on_connect(cs);
        return cs;
    }

    /// ref_count ->
    /// takes a connectable_observable source and uses a ref_count of the subscribers
    /// to control the connection to the published source. The first subscription
    /// will cause a call to connect() and the last unsubscribe will unsubscribe the
    /// connection.
    ///
    auto ref_count() const
        ->      observable<T,   rxo::detail::ref_count<T, this_type>> {
        return  observable<T,   rxo::detail::ref_count<T, this_type>>(
                                rxo::detail::ref_count<T, this_type>(*this));
    }

    /// connect_now ->
    /// takes a connectable_observable source and calls connect during
    /// the construction of the expression. This means that the source
    /// starts running without any subscribers and continues running
    /// after all subscriptions have been unsubscribed.
    ///
    auto connect_now() const
        ->      observable<T,   rxo::detail::connect_now<T, this_type>> {
        return  observable<T,   rxo::detail::connect_now<T, this_type>>(
                                rxo::detail::connect_now<T, this_type>(*this));
    }
};


}

#endif
