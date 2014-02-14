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

    typedef scheduler_base::clock clock;
    typedef time_action<clock::time_point> item_type;

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
        if (!current_thread_queue()) {
            abort();
        }
        current_thread_queue()->queue.pop();
    }
    static void push(item_type item) {
        if (!current_thread_queue()) {
            abort();
        }
        current_thread_queue()->queue.push(std::move(item));
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

struct current_thread : public scheduler_base
{
private:
    typedef current_thread this_type;
    current_thread(const this_type&);

    typedef detail::action_queue queue;

    struct derecurser : public scheduler_base
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

        virtual clock::time_point now() {
            return clock::now();
        }

        virtual void schedule(action a) {
            queue::push(queue::item_type(now(), std::move(a)));
        }

        virtual void schedule(clock::duration when, action a) {
            queue::push(queue::item_type(now() + when, std::move(a)));
        }

        virtual void schedule(clock::time_point when, action a) {
            queue::push(queue::item_type(when, std::move(a)));
        }
    };

public:
    current_thread()
    {
    }
    virtual ~current_thread()
    {
    }

    static bool is_schedule_required() { return !queue::get_scheduler(); }

    virtual clock::time_point now() {
        return clock::now();
    }

    virtual void schedule(action a) {
        schedule(now(), std::move(a));
    }

    virtual void schedule(clock::duration when, action a) {
        schedule(now() + when, std::move(a));
    }

    virtual void schedule(clock::time_point when, action a) {
        if (!a->is_subscribed()) {
            return;
        }

        auto sc = queue::get_scheduler();
        // check ownership
        if (!!sc)
        {
            // already has an owner - delegate
            return sc->schedule(when, std::move(a));
        }

        // take ownership

        sc = queue::ensure(std::make_shared<derecurser>());
        RXCPP_UNWIND_AUTO([]{
            queue::destroy();
        });

        queue::push(queue::item_type(when, std::move(a)));

        // loop until queue is empty
        for (
             auto when = queue::top().when;
             std::this_thread::sleep_until(when), true;
             when = queue::top().when
             )
        {
            auto a = queue::top().a;

            queue::pop();

            while (a->is_subscribed()) {
                a = (*a)(sc);
                if (!queue::empty()) {
                    // take proper place in line
                    sc->schedule(std::move(a));
                    break;
                }
                // tail recurse to a
            }

            if (queue::empty()) {
                break;
            }
        }
    }
};

inline scheduler make_current_thread() {
    return std::make_shared<current_thread>();
}

}

}

#endif
