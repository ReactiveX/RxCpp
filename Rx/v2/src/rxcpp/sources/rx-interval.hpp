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

        auto selectedDest = on_exception(
            [&](){return coordinator.out(o);},
            o);
        if (selectedDest.empty()) {
            return;
        }

        auto counter = std::make_shared<long>(0);

        coordinator.get_output().get_worker().schedule_periodically(
            initial.initial,
            initial.period,
            [selectedDest, counter](const rxsc::schedulable&) {
                // send next value
                selectedDest.get().on_next(++(*counter));
            });
    }
};

}
template<class Duration>
static auto interval(rxsc::scheduler::clock_type::time_point initial, Duration period)
    ->      observable<long,    rxs::detail::interval<identity_one_worker>> {
    return  observable<long,    rxs::detail::interval<identity_one_worker>>(
                                rxs::detail::interval<identity_one_worker>(initial, period, identity_one_worker(rxsc::make_current_thread())));
    static_assert(std::is_same<Duration, rxsc::scheduler::clock_type::duration>::value, "duration must be rxsc::scheduler::clock_type::duration");
}
template<class Coordination>
static auto interval(rxsc::scheduler::clock_type::time_point initial, rxsc::scheduler::clock_type::duration period, Coordination cn)
    ->      observable<long,    rxs::detail::interval<Coordination>> {
    return  observable<long,    rxs::detail::interval<Coordination>>(
                                rxs::detail::interval<Coordination>(initial, period, std::move(cn)));
}

}

}

#endif
