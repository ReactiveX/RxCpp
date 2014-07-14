// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_INTERVAL_HPP)
#define RXCPP_SOURCES_RX_INTERVAL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class Coordination>
struct interval : public source_base<long>
{
    typedef interval<Coordination> this_type;

    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

    struct interval_initial_type
    {
        interval_initial_type(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, coordination_type cn)
            : initial(i)
            , period(p)
            , coordination(std::move(cn))
        {
        }
        rxsc::scheduler::clock_type::time_point initial;
        rxsc::scheduler::clock_type::duration period;
        coordination_type coordination;
    };
    interval_initial_type initial;

    interval(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, coordination_type cn)
        : initial(i, p, std::move(cn))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef typename coordinator_type::template get<Subscriber>::type output_type;

        // creates a worker whose lifetime is the same as this subscription
        auto coordinator = initial.coordination.create_coordinator(o.get_subscription());

        auto controller = coordinator.get_worker();

        auto counter = std::make_shared<long>(0);

        auto producer = [o, counter](const rxsc::schedulable&) {
            // send next value
            o.on_next(++(*counter));
        };

        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            o);
        if (selectedProducer.empty()) {
            return;
        }

        controller.schedule_periodically(initial.initial, initial.period, selectedProducer.get());
    }
};

template<class TimePoint, class Coordination>
struct defer_interval : public defer_observable<
    rxu::all_true<
        std::is_convertible<TimePoint, rxsc::scheduler::clock_type::time_point>::value,
        is_coordination<Coordination>::value>,
    void,
    interval, Coordination>
{
};

}

template<class TimePoint>
auto interval(TimePoint when)
    ->  typename std::enable_if<
                    detail::defer_interval<TimePoint, identity_one_worker>::value,
        typename    detail::defer_interval<TimePoint, identity_one_worker>::observable_type>::type {
    return          detail::defer_interval<TimePoint, identity_one_worker>::make(when, rxsc::scheduler::clock_type::duration::max(), identity_current_thread());
}

template<class Coordination>
auto interval(rxsc::scheduler::clock_type::time_point when, Coordination cn)
    ->  typename std::enable_if<
                    detail::defer_interval<rxsc::scheduler::clock_type::time_point, Coordination>::value,
        typename    detail::defer_interval<rxsc::scheduler::clock_type::time_point, Coordination>::observable_type>::type {
    return          detail::defer_interval<rxsc::scheduler::clock_type::time_point, Coordination>::make(when, rxsc::scheduler::clock_type::duration::max(), std::move(cn));
}

template<class TimePoint>
auto interval(TimePoint when, rxsc::scheduler::clock_type::duration period)
    ->  typename std::enable_if<
                    detail::defer_interval<TimePoint, identity_one_worker>::value,
        typename    detail::defer_interval<TimePoint, identity_one_worker>::observable_type>::type {
    return          detail::defer_interval<TimePoint, identity_one_worker>::make(when, period, identity_current_thread());
}

template<class Coordination>
auto interval(rxsc::scheduler::clock_type::time_point when, rxsc::scheduler::clock_type::duration period, Coordination cn)
    ->  typename std::enable_if<
                    detail::defer_interval<rxsc::scheduler::clock_type::time_point, Coordination>::value,
        typename    detail::defer_interval<rxsc::scheduler::clock_type::time_point, Coordination>::observable_type>::type {
    return          detail::defer_interval<rxsc::scheduler::clock_type::time_point, Coordination>::make(when, period, std::move(cn));
}

}

}

#endif
