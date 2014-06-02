// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_INTERVAL_HPP)
#define RXCPP_SOURCES_RX_INTERVAL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

struct interval : public source_base<long>
{
    typedef interval this_type;

    struct interval_initial_type
    {
        interval_initial_type(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, rxsc::scheduler sc)
            : initial(i)
            , period(p)
            , factory(sc)
        {
        }
        rxsc::scheduler::clock_type::time_point initial;
        rxsc::scheduler::clock_type::duration period;
        rxsc::scheduler factory;
    };
    interval_initial_type initial;

    interval(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, rxsc::scheduler sc)
        : initial(i, p, sc)
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) {

        auto controller = initial.factory.create_worker(o.get_subscription());
        auto counter = std::make_shared<long>(0);

        controller.schedule_periodically(
            initial.initial,
            initial.period,
            [o, counter](const rxsc::schedulable&) {
                // send next value
                o.on_next(++(*counter));
            });
    }
};

}
/*
auto interval(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, rxsc::scheduler sc = rxsc::make_current_thread())
    ->      observable<long, detail::interval> {
    return  observable<long, detail::interval>(detail::interval(i, p, sc));
}
*/
}

}

#endif
