// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SYNCHRONIZE_HPP)
#define RXCPP_RX_SYNCHRONIZE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {

template<class T, class Coordination>
class synchronize_observer : public detail::multicast_observer<T>
{
    using this_type = synchronize_observer<T, Coordination>;
    using base_type = detail::multicast_observer<T>;

    using coordination_type = rxu::decay_t<Coordination>;
    using coordinator_type = typename coordination_type::coordinator_type;
    using output_type = typename coordinator_type::template get<subscriber < T>>::type;

    struct synchronize_observer_state : public std::enable_shared_from_this<synchronize_observer_state>
    {
        using notification_type = rxn::notification<T>;
        using base_notification_type = typename notification_type::type;
        using queue_type = std::deque<base_notification_type>;

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
        mutable queue_type fill_queue;
        composite_subscription lifetime;
        mutable typename mode::type current;
        coordinator_type coordinator;
        output_type destination;

        void ensure_processing(std::unique_lock<std::mutex>& guard) const {
            if (!guard.owns_lock()) {
                std::terminate();
            }
            if (current == mode::Empty) {
                current = mode::Processing;
                auto keepAlive = this->shared_from_this();

                auto drain_queue = [keepAlive, this](const rxsc::schedulable& self){
                    RXCPP_TRY {
                        std::unique_lock<std::mutex> guard(lock);
                        if (!destination.is_subscribed()) {
                            current = mode::Disposed;
                            fill_queue.clear();
                            guard.unlock();
                            lifetime.unsubscribe();
                            return;
                        }
                        if (fill_queue.empty()) {
                            current = mode::Empty;
                            return;
                        }
                        auto notification = std::move(fill_queue.front());
                        fill_queue.pop_front();
                        guard.unlock();
                        std::move(*notification).accept(destination);
                        self();
                    } RXCPP_CATCH(...) {
                        destination.on_error(rxu::current_exception());
                        std::unique_lock<std::mutex> guard(lock);
                        current = mode::Empty;
                    }
                };

                auto selectedDrain = on_exception(
                    [&](){return coordinator.act(drain_queue);},
                    destination);
                if (selectedDrain.empty()) {
                    return;
                }

                auto processor = coordinator.get_worker();
                guard.unlock();
                processor.schedule(lifetime, selectedDrain.get());
            }
        }

        synchronize_observer_state(coordinator_type coor, composite_subscription cs, output_type scbr)
            : lifetime(std::move(cs))
            , current(mode::Empty)
            , coordinator(std::move(coor))
            , destination(std::move(scbr))
        {
        }

        template<class V>
        void on_next(V v) const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                fill_queue.push_back(notification_type::on_next(std::move(v)));
                ensure_processing(guard);
            }
            wake.notify_one();
        }
        void on_error(rxu::error_ptr e) const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                fill_queue.push_back(notification_type::on_error(e));
                ensure_processing(guard);
            }
            wake.notify_one();
        }
        void on_completed() const {
            if (lifetime.is_subscribed()) {
                std::unique_lock<std::mutex> guard(lock);
                fill_queue.push_back(notification_type::on_completed());
                ensure_processing(guard);
            }
            wake.notify_one();
        }
    };

    std::shared_ptr<synchronize_observer_state> state;

public:
    synchronize_observer(coordination_type cn, composite_subscription dl, composite_subscription il)
        : base_type(dl)
    {
        auto o = make_subscriber<T>(dl, make_observer_dynamic<T>( *static_cast<base_type*>(this) ));

        // creates a worker whose lifetime is the same as the destination subscription
        auto coordinator = cn.create_coordinator(dl);

        state = std::make_shared<synchronize_observer_state>(std::move(coordinator), std::move(il), std::move(o));
    }

    subscriber<T> get_subscriber() const {
        return make_subscriber<T>(this->get_id(), state->lifetime, observer<T, detail::synchronize_observer<T, Coordination>>(*this)).as_dynamic();
    }

    template<class V>
    void on_next(V v) const {
        state->on_next(std::move(v));
    }
    void on_error(rxu::error_ptr e) const {
        state->on_error(e);
    }
    void on_completed() const {
        state->on_completed();
    }
};

}

template<class T, class Coordination>
class synchronize
{
    detail::synchronize_observer<T, Coordination> s;

public:
    explicit synchronize(Coordination cn, composite_subscription cs = composite_subscription())
        : s(std::move(cn), std::move(cs), composite_subscription())
    {
    }

    bool has_observers() const {
        return s.has_observers();
    }

    subscriber<T> get_subscriber() const {
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

class synchronize_in_one_worker : public coordination_base
{
    rxsc::scheduler factory;

    class input_type
    {
        rxsc::worker controller;
        rxsc::scheduler factory;
        identity_one_worker coordination;
    public:
        explicit input_type(rxsc::worker w)
            : controller(w)
            , factory(rxsc::make_same_worker(w))
            , coordination(factory)
        {
        }
        inline rxsc::worker get_worker() const {
            return controller;
        }
        inline rxsc::scheduler get_scheduler() const {
            return factory;
        }
        inline rxsc::scheduler::clock_type::time_point now() const {
            return factory.now();
        }
        template<class Observable>
        auto in(Observable o) const
            -> decltype(o.publish_synchronized(coordination).ref_count()) {
            return      o.publish_synchronized(coordination).ref_count();
        }
        template<class Subscriber>
        auto out(Subscriber s) const
            -> Subscriber {
            return s;
        }
        template<class F>
        auto act(F f) const
            -> F {
            return f;
        }
    };

public:

    explicit synchronize_in_one_worker(rxsc::scheduler sc) : factory(sc) {}

    using coordinator_type = coordinator<input_type>;

    inline rxsc::scheduler::clock_type::time_point now() const {
        return factory.now();
    }

    inline coordinator_type create_coordinator(composite_subscription cs = composite_subscription()) const {
        auto w = factory.create_worker(std::move(cs));
        return coordinator_type(input_type(std::move(w)));
    }
};

inline synchronize_in_one_worker synchronize_event_loop() {
    static synchronize_in_one_worker r(rxsc::make_event_loop());
    return r;
}

inline synchronize_in_one_worker synchronize_new_thread() {
    static synchronize_in_one_worker r(rxsc::make_new_thread());
    return r;
}

}

#endif
