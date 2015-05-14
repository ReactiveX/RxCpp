// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_TIMER_HPP)
#define RXCPP_SOURCES_RX_TIMER_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class Coordination>
struct timer : public source_base<long>
{
    typedef timer<Coordination> this_type;

    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

    struct timer_initial_type
    {
        timer_initial_type(rxsc::scheduler::clock_type::time_point t, coordination_type cn)
            : when(t)
            , coordination(std::move(cn))
        {
        }
        rxsc::scheduler::clock_type::time_point when;
        coordination_type coordination;
    };
    timer_initial_type initial;

    timer(rxsc::scheduler::clock_type::time_point t, coordination_type cn)
        : initial(t, std::move(cn))
    {
    }
    timer(rxsc::scheduler::clock_type::duration p, coordination_type cn)
        : initial(rxsc::scheduler::clock_type::time_point(), std::move(cn))
    {
        initial.when = initial.coordination.now() + p;
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        // creates a worker whose lifetime is the same as this subscription
        auto coordinator = initial.coordination.create_coordinator(o.get_subscription());
        auto controller = coordinator.get_worker();

        auto producer = [o](const rxsc::schedulable&) {
            // send the value and complete
            o.on_next(1L);
            o.on_completed();
        };

        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            o);
        if (selectedProducer.empty()) {
            return;
        }

        controller.schedule(initial.when, selectedProducer.get());
    }
};

template<class TimePointOrDuration, class Coordination>
struct defer_timer : public defer_observable<
    rxu::all_true<
        std::is_convertible<TimePointOrDuration, rxsc::scheduler::clock_type::time_point>::value || 
        std::is_convertible<TimePointOrDuration, rxsc::scheduler::clock_type::duration>::value,
        is_coordination<Coordination>::value>,
    void,
    timer, Coordination>
{
};

}

template<class TimePointOrDuration>
auto timer(TimePointOrDuration when)
    ->  typename std::enable_if<
                    detail::defer_timer<TimePointOrDuration, identity_one_worker>::value,
        typename    detail::defer_timer<TimePointOrDuration, identity_one_worker>::observable_type>::type {
    return          detail::defer_timer<TimePointOrDuration, identity_one_worker>::make(when, identity_current_thread());
}

template<class TimePointOrDuration, class Coordination>
auto timer(TimePointOrDuration when, Coordination cn)
    ->  typename std::enable_if<
                    detail::defer_timer<TimePointOrDuration, Coordination>::value,
        typename    detail::defer_timer<TimePointOrDuration, Coordination>::observable_type>::type {
    return          detail::defer_timer<TimePointOrDuration, Coordination>::make(when, std::move(cn));
}

}

}

#endif
