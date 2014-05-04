// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_NEW_THREAD_HPP)
#define RXCPP_RX_SCHEDULER_NEW_THREAD_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

typedef std::function<std::thread(std::function<void()>)> thread_factory;

struct new_thread : public scheduler_interface
{
private:
    typedef new_thread this_type;
    new_thread(const this_type&);

    struct new_worker : public worker_interface
    {
    private:
        typedef new_worker this_type;
        new_worker(const this_type&);

        typedef detail::schedulable_queue<
            typename clock_type::time_point> queue_item_time;

        typedef queue_item_time::item_type item_type;

        composite_subscription lifetime;
        mutable std::mutex lock;
        mutable std::condition_variable wake;
        mutable queue_item_time queue;
        std::thread worker;
        recursion r;

    public:
        virtual ~new_worker()
        {
            {
                std::unique_lock<std::mutex> guard(lock);
                lifetime.unsubscribe();
            }
            worker.join();
        }
        new_worker(composite_subscription cs, thread_factory& tf)
            : lifetime(cs)
        {
            lifetime.add(make_subscription([this](){
                wake.notify_one();
            }));

            worker = tf([this](){
                for(;;) {
                    std::unique_lock<std::mutex> guard(lock);
                    if (queue.empty()) {
                        wake.wait(guard, [this](){
                            return !lifetime.is_subscribed() || !queue.empty();
                        });
                    }
                    if (!lifetime.is_subscribed()) {
                        break;
                    }
                    auto& peek = queue.top();
                    if (!peek.what.is_subscribed()) {
                        queue.pop();
                        continue;
                    }
                    if (clock_type::now() < peek.when) {
                        wake.wait_until(guard, peek.when);
                        continue;
                    }
                    auto what = peek.what;
                    queue.pop();
                    r.reset(queue.empty());
                    guard.unlock();
                    what(r.get_recurse());
                }
            });
        }

        virtual clock_type::time_point now() const {
            return clock_type::now();
        }

        virtual void schedule(const schedulable& scbl) const {
            schedule(now(), scbl);
        }

        virtual void schedule(clock_type::time_point when, const schedulable& scbl) const {
            if (scbl.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push(item_type(when, scbl));
                r.reset(false);
            }
            wake.notify_one();
        }
    };

    mutable thread_factory factory;

public:
    new_thread()
        : factory([](std::function<void()> start){
            return std::thread(std::move(start));
        })
    {
    }
    explicit new_thread(thread_factory tf)
        : factory(tf)
    {
    }
    virtual ~new_thread()
    {
    }

    virtual clock_type::time_point now() const {
        return clock_type::now();
    }

    virtual worker create_worker(composite_subscription cs) const {
        return worker(cs, std::shared_ptr<new_worker>(new new_worker(cs, factory)));
    }
};

inline scheduler make_new_thread() {
    return make_scheduler<new_thread>();
}
inline scheduler make_new_thread(thread_factory tf) {
    return make_scheduler<new_thread>(tf);
}

}

}

#endif
