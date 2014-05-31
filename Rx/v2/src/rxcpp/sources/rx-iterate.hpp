// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_ITERATE_HPP)
#define RXCPP_SOURCES_RX_ITERATE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class Collection>
struct iterate_traits
{
    typedef typename std::decay<Collection>::type collection_type;
    typedef decltype(std::begin(*(collection_type*)nullptr)) iterator_type;
    typedef typename std::iterator_traits<iterator_type>::value_type value_type;
};

template<class Collection>
struct iterate : public source_base<typename iterate_traits<Collection>::value_type>
{
    typedef iterate<Collection> this_type;
    typedef iterate_traits<Collection> traits;

    typedef typename traits::collection_type collection_type;
    typedef typename traits::iterator_type iterator_type;

    struct iterate_initial_type
    {
        iterate_initial_type(collection_type c, rxsc::scheduler sc)
            : collection(std::move(c))
            , factory(sc)
        {
        }
        collection_type collection;
        rxsc::scheduler factory;
    };
    iterate_initial_type initial;

    iterate(collection_type c, rxsc::scheduler sc)
        : initial(std::move(c), sc)
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) {
        struct iterate_state_type
            : public iterate_initial_type
            , public std::enable_shared_from_this<iterate_state_type>
        {
            iterate_state_type(iterate_initial_type i, Subscriber o)
                : iterate_initial_type(i)
                // creates a worker whose lifetime is the same as this subscription
                , controller(iterate_initial_type::factory.create_worker(o.get_subscription()))
                , cursor(std::begin(iterate_initial_type::collection))
                , end(std::end(iterate_initial_type::collection))
                , out(o)
            {
            }
            rxsc::worker controller;
            iterator_type cursor;
            iterator_type end;
            Subscriber out;
        };
        auto state = std::make_shared<iterate_state_type>(initial, o);

        state->controller.schedule(
            [state](const rxsc::schedulable& self){
                if (!state->out.is_subscribed()) {
                    // terminate loop
                    return;
                }

                if (state->cursor != state->end) {
                    // send next value
                    state->out.on_next(*state->cursor);
                    ++state->cursor;
                }

                if (state->cursor == state->end) {
                    state->out.on_completed();
                    // o is unsubscribed
                    return;
                }

                // tail recurse this same action to continue loop
                self();
            });
    }
};

}

template<class Collection>
auto iterate(Collection c, rxsc::scheduler sc = rxsc::make_current_thread())
    ->      observable<typename detail::iterate_traits<Collection>::value_type, detail::iterate<Collection>> {
    return  observable<typename detail::iterate_traits<Collection>::value_type, detail::iterate<Collection>>(
                                                                                detail::iterate<Collection>(std::move(c), sc));
}

}

}

#endif
