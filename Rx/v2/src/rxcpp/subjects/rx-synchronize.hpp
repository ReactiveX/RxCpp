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
                        if (!destination.is_subscribed()) {
                            current = mode::Disposed;
                            queue.clear();
                            guard.unlock();
                            lifetime.unsubscribe();
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
            : lifetime(std::move(cs))
            , processor(std::move(w))
            , current(mode::Empty)
            , destination(std::move(scbr))
        {
        }

        template<class V>
        void on_next(V v) const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push_back(notification_type::on_next(std::move(v)));
                ensure_processing(guard);
            }
            wake.notify_one();
        }
        void on_error(std::exception_ptr e) const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push_back(notification_type::on_error(e));
                ensure_processing(guard);
            }
            wake.notify_one();
        }
        void on_completed() const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                queue.push_back(notification_type::on_completed());
                ensure_processing(guard);
            }
            wake.notify_one();
        }
    };

    std::shared_ptr<synchronize_observer_state> state;

public:
    synchronize_observer(rxsc::worker w, composite_subscription dl, composite_subscription il)
        : base_type(dl)
        , state(std::make_shared<synchronize_observer_state>(
            std::move(w), std::move(il), make_subscriber<T>(dl, make_observer_dynamic<T>( *static_cast<base_type*>(this) ))))
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
    composite_subscription lifetime;
    detail::synchronize_observer<T> s;

public:
    explicit synchronize(rxsc::worker w, composite_subscription cs = composite_subscription())
        : lifetime(composite_subscription())
        , s(std::move(w), std::move(cs), lifetime)
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

}

class syncronize_in_one_worker : public coordination_base
{
    rxsc::scheduler factory;

    class input_type
    {
        rxsc::worker controller;
        rxsc::scheduler factory;
    public:
        explicit input_type(rxsc::worker w)
            : controller(w)
            , factory(rxsc::make_same_worker(w))
        {
        }
        rxsc::worker get_worker() const {
            return controller;
        }
        rxsc::scheduler get_scheduler() const {
            return factory;
        }
        template<class Observable>
        auto operator()(Observable o) const
            -> decltype(o.synchronize(controller).ref_count()) {
            return      o.synchronize(controller).ref_count();
            static_assert(is_observable<Observable>::value, "can only synchronize observables");
        }
    };

    class output_type
    {
        rxsc::worker controller;
        rxsc::scheduler factory;
    public:
        explicit output_type(rxsc::worker w)
            : controller(w)
            , factory(rxsc::make_same_worker(w))
        {
        }
        rxsc::worker get_worker() const {
            return controller;
        }
        rxsc::scheduler get_scheduler() const {
            return factory;
        }
        template<class Subscriber>
        Subscriber operator()(Subscriber s) const {
            return std::move(s);
            static_assert(is_subscriber<Subscriber>::value, "can only synchronize subscribers");
        }
    };

public:

    explicit syncronize_in_one_worker(rxsc::scheduler sc) : factory(sc) {}

    typedef coordinator<input_type, output_type> coordinator_type;

    coordinator_type create_coordinator() const {
        auto w = factory.create_worker();
        return coordinator_type(input_type(w), output_type(w));
    }
};

}

#endif
