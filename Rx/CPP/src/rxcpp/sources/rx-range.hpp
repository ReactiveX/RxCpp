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
        size_t remaining;
        ptrdiff_t step;
        rxsc::scheduler sc;
    };
    state_type init;
    range(T b, size_t c, ptrdiff_t s, rxsc::scheduler sc)
    {
        init.next = b;
        init.remaining = c;
        init.step = s;
        init.sc = sc;
    }
    template<class I>
    void on_subscribe(observer<T, I> o) {
        auto state = std::make_shared<state_type>(init);
        state->sc->schedule([=](rxsc::action that, rxsc::scheduler){
            if (state->remaining == 0) {
                o.on_completed();
                // o is unsubscribed
            }
            if (!o.is_subscribed()) {
                // terminate loop
                return rxsc::make_action_empty();
            }

            // send next value
            --state->remaining;
            o.on_next(state->next);
            state->next = static_cast<T>(state->step + state->next);

            // tail recurse this same action to continue loop
            return that;
        });
    }
};

}

template<class T>
auto range(T start = 0, size_t count = std::numeric_limits<size_t>::max(), ptrdiff_t step = 1, rxsc::scheduler sc = rxsc::make_current_thread())
    ->      observable<T,   detail::range<T>> {
    return  observable<T,   detail::range<T>>(
                            detail::range<T>(start, count, step, sc));
}

}

}

#endif
