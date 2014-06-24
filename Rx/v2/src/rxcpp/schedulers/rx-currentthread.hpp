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
    typedef schedulable_queue<item_type::time_point_type> queue_item_time;

public:
    struct current_thread_queue_type {
        std::shared_ptr<worker_interface> w;
        recursion r;
        queue_item_time queue;
    };

private:
    static current_thread_queue_type*& current_thread_queue() {
        static RXCPP_THREAD_LOCAL current_thread_queue_type* queue;
        return queue;
    }

public:

    static bool owned() {
        return !!current_thread_queue();
    }
    static const std::shared_ptr<worker_interface>& get_worker_interface() {
        return current_thread_queue()->w;
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
    static std::shared_ptr<worker_interface> ensure(std::shared_ptr<worker_interface> w) {
        if (!!current_thread_queue()) {
            abort();
        }
        // create and publish new queue
        current_thread_queue() = new current_thread_queue_type();
        current_thread_queue()->w = w;
        return w;
    }
    static std::unique_ptr<current_thread_queue_type> create(std::shared_ptr<worker_interface> w) {
        std::unique_ptr<current_thread_queue_type> result(new current_thread_queue_type());
        result->w = std::move(w);
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

    struct derecurser : public worker_interface
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

        virtual void schedule(clock_type::time_point when, const schedulable& scbl) const {
            queue::push(queue::item_type(when, scbl));
        }
    };

    struct current_worker : public worker_interface
    {
    private:
        typedef current_thread this_type;
        current_worker(const this_type&);
    public:
        current_worker()
        {
        }
        virtual ~current_worker()
        {
        }

        virtual clock_type::time_point now() const {
            return clock_type::now();
        }

        virtual void schedule(const schedulable& scbl) const {
            schedule(now(), scbl);
        }

        virtual void schedule(clock_type::time_point when, const schedulable& scbl) const {
            if (!scbl.is_subscribed()) {
                return;
            }

            {
                // check ownership
                if (queue::owned()) {
                    // already has an owner - delegate
                    queue::get_worker_interface()->schedule(when, scbl);
                    return;
                }

                // take ownership
                queue::ensure(std::make_shared<derecurser>());
            }
            // release ownership
            RXCPP_UNWIND_AUTO([]{
                queue::destroy();
            });

            const auto& recursor = queue::get_recursion().get_recurse();
            std::this_thread::sleep_until(when);
            if (scbl.is_subscribed()) {
                scbl(recursor);
            }
            if (queue::empty()) {
                return;
            }

            // loop until queue is empty
            for (
                auto next = queue::top().when;
                (std::this_thread::sleep_until(next), true);
                next = queue::top().when
            ) {
                auto what = queue::top().what;

                queue::pop();

                if (what.is_subscribed()) {
                    what(recursor);
                }

                if (queue::empty()) {
                    break;
                }
            }
        }
    };

    std::shared_ptr<current_worker> wi;

public:
    current_thread()
        : wi(new current_worker())
    {
    }
    virtual ~current_thread()
    {
    }

    static bool is_schedule_required() { return !queue::owned(); }

    inline bool is_tail_recursion_allowed() const {
        return queue::empty();
    }

    virtual clock_type::time_point now() const {
        return clock_type::now();
    }

    virtual worker create_worker(composite_subscription cs) const {
        return worker(std::move(cs), wi);
    }
};

inline const scheduler& make_current_thread() {
    static auto ct = make_scheduler<current_thread>();
    return ct;
}

}

}

#endif
