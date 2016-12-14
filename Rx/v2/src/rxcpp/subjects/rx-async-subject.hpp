// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_ASYNC_SUBJECT_HPP)
#define RXCPP_RX_ASYNC_SUBJECT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {

struct multicast_async_state_type
    : public std::enable_shared_from_this<multicast_async_state_type>
{
    struct mode
    {
        enum type {
            Invalid = 0,
            Casting,
            Disposed,
            Completed,
            Errored
        };
    };

    explicit multicast_async_state_type(composite_subscription cs)
        : generation(0)
        , current(mode::Casting)
        , lifetime(cs)
    {
    }
    std::atomic<int> generation;
    std::mutex lock;
    typename mode::type current;
    std::exception_ptr error;
    composite_subscription lifetime;

    template<class F,class observer_type>
    void map( std::vector<observer_type> &l, F f ) {
        std::unique_lock<std::mutex> guard(lock);
        for (auto& o : l) f(o);
    }

};

}


template<class T>
class async_subject
{
    detail::multicast_observer<T,detail::multicast_async_state_type> s;

public:
    typedef subscriber<T, observer<T, detail::multicast_observer<T,detail::multicast_async_state_type>>> subscriber_type;
    typedef observable<T> observable_type;
    async_subject()
        : s(composite_subscription())
    {
    }
    explicit async_subject(composite_subscription cs)
        : s(cs)
    {
    }

    bool has_observers() const {
        return s.has_observers();
    }

    subscriber_type get_subscriber() const {
        return s.get_subscriber();
    }

    observable<T> get_observable() const {
        auto keepAlive = s;
        return make_observable_dynamic<T>([=](subscriber<T> o){
            keepAlive.add(keepAlive.get_subscriber(), std::move(o));
        });
    }
};

}

}

#endif
