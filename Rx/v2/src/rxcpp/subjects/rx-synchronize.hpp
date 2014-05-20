// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SYNCHRONIZE_HPP)
#define RXCPP_RX_SYNCHRONIZE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {

template<class T>
class synchronize_observer : public detail::multicast_observer<T>
{
    typedef synchronize_observer<T> this_type;
    typedef detail::multicast_observer<T> base_type;

    struct synchronize_observer_state : public std::enable_shared_from_this<synchronize_observer_state>
    {
        typedef rxn::notification<T> notification_type;
        typedef typename notification_type::type base_notification_type;
        typedef std::deque<base_notification_type> queue_type;

        struct mode
        {
            enum type {
                Invalid = 0,
                Processing,
                Empty,
                Disposed
            };
        };

        mutable std::mutex lock;
        mutable std::condition_variable wake;
        mutable queue_type queue;
        composite_subscription lifetime;
        rxsc::worker processor;
        mutable typename mode::type current;
        subscriber<T> destination;

        void ensure_processing(std::unique_lock<std::mutex>& guard) const {
            if (!guard.owns_lock()) {
                abort();
            }
            if (current == mode::Empty) {
                current = mode::Processing;
                auto keepAlive = this->shared_from_this();
                processor.schedule(lifetime, [keepAlive, this](const rxsc::schedulable& self){
                    try {
                        std::unique_lock<std::mutex> guard(lock);
                        if (!lifetime.is_subscribed() || !destination.is_subscribed()) {
                            current = mode::Disposed;
                            queue.clear();
                            guard.unlock();
                            lifetime.unsubscribe();
                            destination.unsubscribe();
                            return;
                        }
                        if (queue.empty()) {
                            current = mode::Empty;
                            return;
                        }
                        auto notification = std::move(queue.front());
                        queue.pop_front();
                        guard.unlock();
                        notification->accept(destination);
                        self();
                    } catch(...) {
                        destination.on_error(std::current_exception());
                        std::unique_lock<std::mutex> guard(lock);
                        current = mode::Empty;
                    }
                });
            }
        }

        synchronize_observer_state(rxsc::worker w, composite_subscription cs, subscriber<T> scbr)
            : lifetime(cs)
            , processor(w)
            , current(mode::Empty)
            , destination(scbr)
        {
        }

        template<class V>
        void on_next(V v) const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push_back(notification_type::on_next(std::move(v)));
                wake.notify_one();
                ensure_processing(guard);
            }
        }
        void on_error(std::exception_ptr e) const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push_back(notification_type::on_error(e));
                wake.notify_one();
                ensure_processing(guard);
            }
        }
        void on_completed() const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push_back(notification_type::on_completed());
                wake.notify_one();
                ensure_processing(guard);
            }
        }
    };

    std::shared_ptr<synchronize_observer_state> state;

public:
    synchronize_observer(rxsc::worker w, composite_subscription cs)
        : base_type(cs)
        , state(std::make_shared<synchronize_observer_state>(
            w, cs, make_subscriber<T>(cs, make_observer_dynamic<T>( *static_cast<base_type*>(this) ))))
    {}

    template<class V>
    void on_next(V v) const {
        state->on_next(std::move(v));
    }
    void on_error(std::exception_ptr e) const {
        state->on_error(e);
    }
    void on_completed() const {
        state->on_completed();
    }
};

}

template<class T>
class synchronize
{
    rxsc::worker controller;
    composite_subscription lifetime;
    detail::synchronize_observer<T> s;

public:
    explicit synchronize(rxsc::worker w, composite_subscription cs = composite_subscription())
        : controller(w)
        , lifetime(cs)
        , s(w, cs)
    {
    }

    bool has_observers() const {
        return s.has_observers();
    }

    subscriber<T> get_subscriber() const {
        return make_subscriber<T>(lifetime, make_observer_dynamic<T>(observer<T, detail::synchronize_observer<T>>(s)));
    }

    observable<T> get_observable() const {
        return make_observable_dynamic<T>([this](subscriber<T> o){
            this->s.add(std::move(o));
        });
    }
};

//
// this type is used by operators that subscribe to
// multiple sources to ensure that the notifications are serialized
//
class synchronize_observable
{
    rxsc::scheduler factory;
    rxsc::worker controller;
public:
    synchronize_observable(rxsc::scheduler sc)
        : factory(sc)
        , controller(sc.create_worker())
    {
    }
    synchronize_observable(const synchronize_observable& o)
        : factory(o.factory)
        // new worker for each copy. this spreads work across threads
        // but keeps each use serialized
        , controller(factory.create_worker())
    {
    }
    synchronize_observable(synchronize_observable&& o)
        : factory(std::move(o.factory))
        , controller(std::move(o.controller))
    {
    }
    synchronize_observable& operator=(synchronize_observable o)
    {
        factory = std::move(o.factory);
        controller = std::move(o.controller);
        return *this;
    }
    template<class Observable>
    auto operator()(Observable o)
        -> decltype(o.synchronize(controller).ref_count()) {
        return      o.synchronize(controller).ref_count();
        static_assert(is_observable<Observable>::value, "can only synchronize observables");
    }
};

}

}

#endif
