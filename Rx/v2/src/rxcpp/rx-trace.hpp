// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_TRACE_HPP)
#define RXCPP_RX_TRACE_HPP

#include <exception>

namespace rxcpp {

struct trace_noop
{
    template<class Worker, class Schedulable>
    inline void schedule_enter(const Worker&, const Schedulable&) {}
    template<class Worker>
    inline void schedule_return(const Worker&) {}
    template<class Worker, class When, class Schedulable>
    inline void schedule_when_enter(const Worker&, const When&, const Schedulable&) {}
    template<class Worker>
    inline void schedule_when_return(const Worker&) {}

    template<class Schedulable>
    inline void action_enter(const Schedulable&) {}
    template<class Schedulable>
    inline void action_return(const Schedulable&) {}
    template<class Schedulable>
    inline void action_recurse(const Schedulable&) {}

    template<class Observable, class Subscriber>
    inline void subscribe_enter(const Observable& o, const Subscriber& s) {}
    template<class Observable>
    inline void subscribe_return(const Observable& o) {}

    template<class OperatorSource, class OperatorChain, class Subscriber, class SubscriberLifted>
    inline void lift_enter(const OperatorSource&, const OperatorChain&, const Subscriber&, const SubscriberLifted&) {}
    template<class OperatorSource, class OperatorChain>
    inline void lift_return(const OperatorSource&, const OperatorChain&) {}

    template<class SubscriptionState>
    inline void unsubscribe_enter(const SubscriptionState&) {}
    template<class SubscriptionState>
    inline void unsubscribe_return(const SubscriptionState&) {}

    template<class SubscriptionState, class Subscription>
    inline void subscription_add_enter(const SubscriptionState&, const Subscription&) {}
    template<class SubscriptionState>
    inline void subscription_add_return(const SubscriptionState&) {}

    template<class SubscriptionState, class WeakSubscription>
    inline void subscription_remove_enter(const SubscriptionState&, const WeakSubscription&) {}
    template<class SubscriptionState>
    inline void subscription_remove_return(const SubscriptionState&) {}

    template<class Subscriber, class T>
    inline void on_next_enter(const Subscriber&, const T&) {}
    template<class Subscriber>
    inline void on_next_return(const Subscriber&) {}

    template<class Subscriber>
    inline void on_error_enter(const Subscriber&, const std::exception_ptr&) {}
    template<class Subscriber>
    inline void on_error_return(const Subscriber&) {}

    template<class Subscriber>
    inline void on_completed_enter(const Subscriber&) {}
    template<class Subscriber>
    inline void on_completed_return(const Subscriber&) {}
};

struct trace_tag {};

}

auto rxcpp_trace_activity(...) -> rxcpp::trace_noop;


#endif
