// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_HPP)
#define RXCPP_RX_SCHEDULER_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

class action_type;
typedef std::shared_ptr<action_type> action;

class scheduler_base;
typedef std::shared_ptr<scheduler_base> scheduler;

class action_type
    : public subscription_base
    , public std::enable_shared_from_this<action_type>
{
    typedef action_type this_type;

public:
    typedef composite_subscription subscription_type;
    typedef subscription_type::weak_subscription weak_subscription;
    typedef std::function<action(action, scheduler)> function_type;

private:
    mutable this_type::subscription_type s;
    function_type f;

    static action shared_empty;

public:
    action_type()
    {
        s.unsubscribe();
    }

    explicit action_type(function_type f)
        : f(std::move(f))
    {
        if (!this->f) {
            s.unsubscribe();
        }
    }
    action_type(this_type::subscription_type s, function_type f)
        : s(std::move(s))
        , f(std::move(f))
    {
        if (!this->f) {
            this->s.unsubscribe();
        }
    }

    inline static action empty() {
        return shared_empty;
    }

    inline action operator()(scheduler sc) {
        if (is_subscribed() && f) {
            return f(this->shared_from_this(), std::move(sc));
        }
        return empty();
    }

    inline bool is_subscribed() const {
        return s.is_subscribed();
    }
    inline this_type::weak_subscription add(dynamic_subscription ds) const {
        return s.add(std::move(ds));
    }
    inline void remove(this_type::weak_subscription ws) const {
        s.remove(ws);
    }
    inline void unsubscribe() const {
        s.unsubscribe();
    }
};

//static
RXCPP_SELECT_ANY action action_type::shared_empty = std::make_shared<action_type>();

inline action make_action_empty() {
    return action_type::empty();
}

inline action make_action(action_type::function_type f) {
    return std::make_shared<action_type>(std::move(f));
}

inline action make_action(action_type::subscription_type s, action_type::function_type f) {
    return std::make_shared<action_type>(std::move(s), std::move(f));
}

class scheduler_base
    : public std::enable_shared_from_this<scheduler_base>
{
    typedef scheduler_base this_type;

public:
    typedef std::chrono::steady_clock clock;

    virtual ~scheduler_base() {}

    virtual clock::time_point now() = 0;

    virtual void schedule(action a) = 0;
    virtual void schedule(clock::duration when, action a) = 0;
    virtual void schedule(clock::time_point when, action a) = 0;

    inline void schedule(action_type::function_type f){
        return schedule(make_action(std::move(f)));
    }
    inline void schedule(clock::duration when, action_type::function_type f){
        return schedule(when, make_action(std::move(f)));
    }
    inline void schedule(clock::time_point when, action_type::function_type f){
        return schedule(when, make_action(std::move(f)));
    }
    inline void schedule(action_type::subscription_type s, action_type::function_type f){
        return schedule(make_action(std::move(s), std::move(f)));
    }
    inline void schedule(clock::duration when, action_type::subscription_type s, action_type::function_type f){
        return schedule(when, make_action(std::move(s), std::move(f)));
    }
    inline void schedule(clock::time_point when, action_type::subscription_type s, action_type::function_type f){
        return schedule(when, make_action(std::move(s), std::move(f)));
    }
};

namespace detail {

template<class TimePoint>
struct time_action
{
    time_action(TimePoint when, action a)
        : when(when)
        , a(std::move(a))
    {
    }
    TimePoint when;
    action a;
};

}

}
namespace rxsc=schedulers;

}

#include "schedulers/rx-currentthread.hpp"

#endif
