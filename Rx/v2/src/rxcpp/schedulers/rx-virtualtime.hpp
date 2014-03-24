// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_VIRTUAL_TIME_HPP)
#define RXCPP_RX_SCHEDULER_VIRTUAL_TIME_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

namespace detail {

template<class Absolute, class Relative>
struct virtual_time_base : public scheduler_interface
{
private:
    typedef virtual_time_base<Absolute, Relative> this_type;
    virtual_time_base(const virtual_time_base&);

    mutable bool isenabled;

public:
    typedef Absolute absolute;
    typedef Relative relative;

    virtual ~virtual_time_base()
    {
    }

protected:
    virtual_time_base()
        : isenabled(false)
        , clock_now(0)
    {
    }
    explicit virtual_time_base(absolute initialClock)
        : isenabled(false)
        , clock_now(initialClock)
    {
    }

    mutable absolute clock_now;

    typedef time_schedulable<long> item_type;

    virtual absolute add(absolute, relative) const =0;

    virtual typename scheduler_base::clock_type::time_point to_time_point(absolute) const =0;
    virtual relative to_relative(typename scheduler_base::clock_type::duration) const =0;

    virtual item_type top() const =0;
    virtual void pop() const =0;
    virtual bool empty() const =0;

public:
    virtual void schedule_absolute(absolute, const schedulable&) const =0;

    virtual void schedule_relative(relative when, const schedulable& a) const {
        auto at = add(clock_now, when);
        return schedule_absolute(at, a);
    }

    bool is_enabled() const {return isenabled;}
    absolute clock() const {return clock_now;}

    void start() const
    {
        if (!isenabled) {
            isenabled = true;
            rxsc::recursion r;
            r.reset(false);
            while (!empty() && isenabled) {
                auto next = top();
                pop();
                if (next.what.is_subscribed()) {
                    if (next.when > clock_now) {
                        clock_now = next.when;
                    }
                    next.what(next.what, r.get_recurse());
                }
                else {
                    isenabled = false;
                }
            }
        }
    }

    void stop() const
    {
        isenabled = false;
    }

    void advance_to(absolute time) const
    {
        if (time < clock_now) {
            abort();
        }

        if (time == clock_now) {
            return;
        }

        if (!isenabled) {
            isenabled = true;
            while (!empty() && isenabled) {
                auto next = top();
                pop();
                if (next.what.is_subscribed() && next.when <= time) {
                    if (next.when > clock_now) {
                        clock_now = next.when;
                    }
                    next.what.get_action()(next.what);
                }
                else {
                    isenabled = false;
                }
            }

            clock_now = time;
        }
        else {
            abort();
        }
    }

    void advance_by(relative time) const
    {
        auto dt = add(clock_now, time);

        if (dt < clock_now) {
            abort();
        }

        if (dt == clock_now) {
            return;
        }

        if (!isenabled) {
            advance_to(dt);
        }
        else {
            abort();
        }
    }

    void sleep(relative time) const
    {
        auto dt = add(clock_now, time);

        if (dt < clock_now) {
            abort();
        }

        clock_now = dt;
    }

    virtual clock_type::time_point now() const {
        return to_time_point(clock_now);
    }

    virtual void schedule(const schedulable& scbl) const {
        schedule_absolute(clock_now, scbl);
    }

    virtual void schedule(clock_type::duration when, const schedulable& scbl) const {
        schedule_absolute(to_relative(when), scbl);
    }

    virtual void schedule(clock_type::time_point when, const schedulable& scbl) const {
        schedule_absolute(to_relative(when - now()), scbl);
    }

};

}

template<class Absolute, class Relative>
class virtual_time : public detail::virtual_time_base<Absolute, Relative>
{
private:
    typedef virtual_time this_type;
    virtual_time(const this_type&);

    typedef detail::virtual_time_base<Absolute, Relative> base;

    typedef typename base::item_type item_type;

    struct compare_item_time
    {
        bool operator()(const item_type& lhs, const item_type& rhs) const {
            return lhs.when > rhs.when;
        }
    };

    typedef std::priority_queue<
        item_type,
        std::vector<item_type>,
        compare_item_time
    > queue_item_time;

    mutable queue_item_time queue;

public:
    virtual ~virtual_time()
    {
    }

protected:
    virtual_time()
    {
    }
    explicit virtual_time(typename base::absolute initialClock)
        : base(initialClock)
    {
    }

    virtual item_type top() const {
        return queue.top();
    }
    virtual void pop() const {
        queue.pop();
    }
    virtual bool empty() const {
        return queue.empty();
    }

    using base::schedule_absolute;
    using base::schedule_relative;

    virtual void schedule_absolute(typename base::absolute when, const schedulable& a) const
    {
        // use a separate subscription here so that a's subscription is not affected
        auto run = make_schedulable(
            scheduler(this->shared_from_this()),
            [a](const schedulable& scbl) {
                rxsc::recursion r;
                r.reset(false);
                if (scbl.is_subscribed()) {
                    scbl.unsubscribe(); // unsubscribe() run, not a;
                    a(a, r.get_recurse());
                }
            });
        queue.push(item_type(when, run));
    }

};


}

}

#endif
