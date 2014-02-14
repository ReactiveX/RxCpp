// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_VIRTUAL_TIME_HPP)
#define RXCPP_RX_SCHEDULER_VIRTUAL_TIME_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

namespace detail {

template<class Absolute, class Relative>
struct virtual_time_base : public scheduler_base
{
private:
    typedef virtual_time_base this_type;
    virtual_time_base(const virtual_time_base&);

    bool isenabled;

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

    absolute clock_now;

    typedef time_action<typename this_type::clock_type::time_point> item_type;

    virtual absolute add(absolute, relative) =0;

    virtual typename this_type::clock_type::time_point to_time_point(absolute) =0;
    virtual relative to_relative(typename this_type::clock_type::duration) =0;

    virtual item_type top() =0;
    virtual void pop() =0;
    virtual bool empty() =0;

public:
    virtual void schedule_absolute(absolute, action) =0;

    virtual void schedule_relative(relative dueTime, action a) {
        auto at = add(clock_now, dueTime);
        return schedule_absolute(at, std::move(a));
    }

    bool is_enabled() {return isenabled;}
    absolute clock() {return clock_now;}

    void start()
    {
        if (!isenabled) {
            isenabled = true;
            while (!empty() && isenabled) {
                auto next = top();
                pop();
                if (next.a->is_subscribed()) {
                    if (next.when > clock_now) {
                        clock_now = next.when;
                    }
                    auto a = (*next.a)(shared_from_this());
                    if (a.subscribed()) {
                        schedule(a);
                    }
                }
                else {
                    isenabled = false;
                }
            }
        }
    }

    void stop()
    {
        isenabled = false;
    }

    void advance_to(absolute time)
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
                if (next.a->is_subscribed() && next.when <= time) {
                    if (next.when > clock_now) {
                        clock_now = next.when;
                    }
                    auto a = (*next.a)(shared_from_this());
                    if (a.subscribed()) {
                        schedule(a);
                    }
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

    void advance_by(relative time)
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

    void sleep(relative time)
    {
        auto dt = add(clock_now, time);

        if (dt < clock_now) {
            abort();
        }

        clock_now = dt;
    }

    virtual clock_type::time_point now() {
        return to_time_point(clock_now);
    }

    virtual void schedule(action a) {
        schedule_absolute(clock_now, std::move(a));
    }

    virtual void schedule(clock_type::duration when, action a) {
        schedule_absolute(to_relative(when), std::move(a));
    }

    virtual void schedule(clock_type::time_point when, action a) {
        schedule_absolute(to_relative(when - now()), std::move(a));
    }

};

}

template<class Absolute, class Relative>
class virtual_time : public detail::virtual_time_base<Absolute, Relative>
{
private:
    virtual_time(const virtual_time&);

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

    queue_item_time queue;

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

    virtual item_type top() {
        return queue.top();
    }
    virtual void pop() {
        queue.pop();
    }
    virtual bool empty() {
        return queue.empty();
    }

    virtual void schedule_absolute(typename base::absolute when, action a)
    {
        // use a separate subscription here so that a's subscription is not affected
        auto run = make_action([a](action that, scheduler sc) {
            if (that->is_subscribed()) {
                that->unsubscribe(); // unsubscribe() run, not a;
                (*a)(sc);
            }
            return make_action_empty();
        });
        queue.push(item_type(when, run));
    }

};


}

}

#endif
