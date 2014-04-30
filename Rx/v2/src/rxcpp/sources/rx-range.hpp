// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_RANGE_HPP)
#define RXCPP_SOURCES_RX_RANGE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class T>
struct range : public source_base<T>
{
    struct state_type
    {
        T next;
        T last;
        ptrdiff_t step;
        rxsc::scheduler sc;
        rxsc::worker w;
    };
    state_type init;
    range(T f, T l, ptrdiff_t s, rxsc::scheduler sc)
    {
        init.next = f;
        init.last = l;
        init.step = s;
        init.sc = sc;
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) {
        auto state = std::make_shared<state_type>(init);
        // creates a worker whose lifetime is the same as this subscription
        state->w = state->sc.create_worker(o.get_subscription());
        state->w.schedule(
            [=](const rxsc::schedulable& self){
                if (!o.is_subscribed()) {
                    // terminate loop
                    return;
                }

                // send next value
                o.on_next(state->next);
                if (std::abs(state->last - state->next) < std::abs(state->step)) {
                    if (state->last != state->next) {
                        o.on_next(state->last);
                    }
                    o.on_completed();
                    // o is unsubscribed
                    return;
                }
                state->next = static_cast<T>(state->step + state->next);

                // tail recurse this same action to continue loop
                self();
            });
    }
};

}

template<class T>
auto range(T first = 0, T last = std::numeric_limits<T>::max(), ptrdiff_t step = 1, rxsc::scheduler sc = rxsc::make_current_thread())
    ->      observable<T,   detail::range<T>> {
    return  observable<T,   detail::range<T>>(
                            detail::range<T>(first, last, step, sc));
}
template<class T>
auto range(T first = 0, rxsc::scheduler sc = rxsc::make_current_thread())
    ->      observable<T,   detail::range<T>> {
    return  observable<T,   detail::range<T>>(
                            detail::range<T>(first, std::numeric_limits<T>::max(), 1, sc));
}

}

}

#endif
