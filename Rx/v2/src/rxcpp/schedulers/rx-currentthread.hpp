// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_CURRENT_THREAD_HPP)
#define RXCPP_RX_SCHEDULER_CURRENT_THREAD_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

namespace detail {

struct action_queue
{
    typedef action_queue this_type;

    typedef scheduler_base::clock_type clock;
    typedef time_schedulable<clock::time_point> item_type;

private:
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

public:
    struct current_thread_queue_type {
        scheduler sc;
        recursion r;
        queue_item_time queue;
    };

private:
    static current_thread_queue_type*& current_thread_queue() {
        RXCPP_THREAD_LOCAL static current_thread_queue_type* queue;
        return queue;
    }

public:

    static scheduler get_scheduler() {
        return !!current_thread_queue() ? current_thread_queue()->sc : scheduler();
    }
    static recursion& get_recursion() {
        return current_thread_queue()->r;
    }
    static bool empty() {
        if (!current_thread_queue()) {
            abort();
        }
        return current_thread_queue()->queue.empty();
    }
    static queue_item_time::const_reference top() {
        if (!current_thread_queue()) {
            abort();
        }
        return current_thread_queue()->queue.top();
    }
    static void pop() {
        auto state = current_thread_queue();
        if (!state) {
            abort();
        }
        state->queue.pop();
        if (state->queue.empty()) {
            // allow recursion
            state->r.reset(true);
        }
    }
    static void push(item_type item) {
        auto state = current_thread_queue();
        if (!state) {
            abort();
        }
        if (!item.what.is_subscribed()) {
            return;
        }
        state->queue.push(std::move(item));
        // disallow recursion
        state->r.reset(false);
    }
    static scheduler ensure(scheduler sc) {
        if (!!current_thread_queue()) {
            abort();
        }
        // create and publish new queue
        current_thread_queue() = new current_thread_queue_type();
        current_thread_queue()->sc = sc;
        return sc;
    }
    static std::unique_ptr<current_thread_queue_type> create(scheduler sc) {
        std::unique_ptr<current_thread_queue_type> result(new current_thread_queue_type());
        result->sc = std::move(sc);
        return result;
    }
    static void set(current_thread_queue_type* queue) {
        if (!!current_thread_queue()) {
            abort();
        }
        // publish new queue
        current_thread_queue() = queue;
    }
    static void destroy(current_thread_queue_type* queue) {
        delete queue;
    }
    static void destroy() {
        if (!current_thread_queue()) {
            abort();
        }
        destroy(current_thread_queue());
        current_thread_queue() = nullptr;
    }
};


}

struct current_thread : public scheduler_interface
{
private:
    typedef current_thread this_type;
    current_thread(const this_type&);

    typedef detail::action_queue queue;

    struct derecurser : public scheduler_interface
    {
    private:
        typedef current_thread this_type;
        derecurser(const this_type&);
    public:
        derecurser()
        {
        }
        virtual ~derecurser()
        {
        }

        virtual clock_type::time_point now() const {
            return clock_type::now();
        }

        virtual void schedule(const schedulable& scbl) const {
            queue::push(queue::item_type(now(), scbl));
        }

        virtual void schedule(clock_type::duration when, const schedulable& scbl) const {
            queue::push(queue::item_type(now() + when, scbl));
        }

        virtual void schedule(clock_type::time_point when, const schedulable& scbl) const {
            queue::push(queue::item_type(when, scbl));
        }
    };

public:
    current_thread()
    {
    }
    virtual ~current_thread()
    {
    }

    static bool is_schedule_required() { return queue::get_scheduler() == scheduler(); }

    virtual clock_type::time_point now() const {
        return clock_type::now();
    }

    inline bool is_tail_recursion_allowed() const {
        return queue::empty();
    }

    virtual void schedule(const schedulable& scbl) const {
        schedule(now(), scbl);
    }

    virtual void schedule(clock_type::duration when, const schedulable& scbl) const {
        schedule(now() + when, scbl);
    }

    virtual void schedule(clock_type::time_point when, const schedulable& scbl) const {
        if (!scbl.is_subscribed()) {
            return;
        }

        auto sc = queue::get_scheduler();
        // check ownership
        if (sc != scheduler())
        {
            // already has an owner - delegate
            return sc.schedule(when, scbl);
        }

        // take ownership

        sc = queue::ensure(make_scheduler<derecurser>());
        RXCPP_UNWIND_AUTO([]{
            queue::destroy();
        });

        queue::push(queue::item_type(when, scbl));

        const auto& recursor = queue::get_recursion().get_recurse();

        // loop until queue is empty
        for (
             auto when = queue::top().when;
             std::this_thread::sleep_until(when), true;
             when = queue::top().when
             )
        {
            auto what = queue::top().what;

            queue::pop();

            what(what, recursor);

            if (queue::empty()) {
                break;
            }
        }
    }
};

inline scheduler make_current_thread() {
    return make_scheduler<current_thread>();
}

}

}

#endif
