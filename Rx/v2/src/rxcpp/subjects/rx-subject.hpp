// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SUBJECT_HPP)
#define RXCPP_RX_SUBJECT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {

template<class T>
class multicast_observer
{
    typedef subscriber<T> observer_type;
    typedef std::vector<observer_type> list_type;

    struct mode
    {
        enum type {
            Invalid = 0,
            Casting,
            Disposed,
            Completed,
            Errored
        };
    };

    struct state_type
        : public std::enable_shared_from_this<state_type>
    {
        explicit state_type(composite_subscription cs)
            : current(mode::Casting)
            , lifetime(cs)
        {
        }
        std::mutex lock;
        typename mode::type current;
        rxu::error_ptr error;
        composite_subscription lifetime;
    };

    struct completer_type
        : public std::enable_shared_from_this<completer_type>
    {
        ~completer_type()
        {
        }
        completer_type(std::shared_ptr<state_type> s, const std::shared_ptr<completer_type>& old, observer_type o)
            : state(s)
        {
            retain(old);
            observers.push_back(o);
        }
        completer_type(std::shared_ptr<state_type> s, const std::shared_ptr<completer_type>& old)
            : state(s)
        {
            retain(old);
        }
        void retain(const std::shared_ptr<completer_type>& old) {
            if (old) {
                observers.reserve(old->observers.size() + 1);
                std::copy_if(
                    old->observers.begin(), old->observers.end(),
                    std::inserter(observers, observers.end()),
                    [](const observer_type& o){
                        return o.is_subscribed();
                    });
            }
        }
        std::shared_ptr<state_type> state;
        list_type observers;
    };

    // this type prevents a circular ref between state and completer
    struct binder_type
        : public std::enable_shared_from_this<binder_type>
    {
        explicit binder_type(composite_subscription cs)
            : state(std::make_shared<state_type>(cs))
            , id(trace_id::make_next_id_subscriber())
        {
        }

        std::shared_ptr<state_type> state;

        trace_id id;

        // used to avoid taking lock in on_next
        mutable std::weak_ptr<completer_type> current_completer;

        // must only be accessed under state->lock
        mutable std::shared_ptr<completer_type> completer;
    };

    std::shared_ptr<binder_type> b;

public:
    typedef subscriber<T, observer<T, detail::multicast_observer<T>>> input_subscriber_type;

    explicit multicast_observer(composite_subscription cs)
        : b(std::make_shared<binder_type>(cs))
    {
        std::weak_ptr<binder_type> binder = b;
        b->state->lifetime.add([binder](){
            auto b = binder.lock();
            if (b && b->state->current == mode::Casting){
                b->state->current = mode::Disposed;
                b->current_completer.reset();
                b->completer.reset();
            }
        });
    }
    trace_id get_id() const {
        return b->id;
    }
    composite_subscription get_subscription() const {
        return b->state->lifetime;
    }
    input_subscriber_type get_subscriber() const {
        return make_subscriber<T>(get_id(), get_subscription(), observer<T, detail::multicast_observer<T>>(*this));
    }
    bool has_observers() const {
        std::unique_lock<std::mutex> guard(b->state->lock);
        return b->completer && !b->completer->observers.empty();
    }
    template<class SubscriberFrom>
    void add(const SubscriberFrom& sf, observer_type o) const {
        trace_activity().connect(sf, o);
        std::unique_lock<std::mutex> guard(b->state->lock);
        switch (b->state->current) {
        case mode::Casting:
            {
                if (o.is_subscribed()) {
                    std::weak_ptr<binder_type> binder = b;
                    o.add([=](){
                        auto b = binder.lock();
                        if (b) {
                            std::unique_lock<std::mutex> guard(b->state->lock);
                            b->completer = std::make_shared<completer_type>(b->state, b->completer);
                        }
                    });
                    b->completer = std::make_shared<completer_type>(b->state, b->completer, o);
                }
            }
            break;
        case mode::Completed:
            {
                guard.unlock();
                o.on_completed();
                return;
            }
            break;
        case mode::Errored:
            {
                auto e = b->state->error;
                guard.unlock();
                o.on_error(e);
                return;
            }
            break;
        case mode::Disposed:
            {
                guard.unlock();
                o.unsubscribe();
                return;
            }
            break;
        default:
            std::terminate();
        }
    }
    void on_next(const T& v) const {
        auto current_completer = b->current_completer.lock();
        if (!current_completer) {
            std::unique_lock<std::mutex> guard(b->state->lock);
            b->current_completer = b->completer;
            current_completer = b->current_completer.lock();
        }
        if (!current_completer || current_completer->observers.empty()) {
            return;
        }
        for (auto& o : current_completer->observers) {
            if (o.is_subscribed()) {
                o.on_next(v);
            }
        }
    }
    void on_error(rxu::error_ptr e) const {
        std::unique_lock<std::mutex> guard(b->state->lock);
        if (b->state->current == mode::Casting) {
            b->state->error = e;
            b->state->current = mode::Errored;
            auto s = b->state->lifetime;
            auto c = std::move(b->completer);
            b->current_completer.reset();
            guard.unlock();
            if (c) {
                for (auto& o : c->observers) {
                    if (o.is_subscribed()) {
                        o.on_error(e);
                    }
                }
            }
            s.unsubscribe();
        }
    }
    void on_completed() const {
        std::unique_lock<std::mutex> guard(b->state->lock);
        if (b->state->current == mode::Casting) {
            b->state->current = mode::Completed;
            auto s = b->state->lifetime;
            auto c = std::move(b->completer);
            b->current_completer.reset();
            guard.unlock();
            if (c) {
                for (auto& o : c->observers) {
                    if (o.is_subscribed()) {
                        o.on_completed();
                    }
                }
            }
            s.unsubscribe();
        }
    }
};


}

template<class T>
class subject
{
    detail::multicast_observer<T> s;

public:
    typedef subscriber<T, observer<T, detail::multicast_observer<T>>> subscriber_type;
    typedef observable<T> observable_type;
    subject()
        : s(composite_subscription())
    {
    }
    explicit subject(composite_subscription cs)
        : s(cs)
    {
    }

    bool has_observers() const {
        return s.has_observers();
    }

    composite_subscription get_subscription() const {
        return s.get_subscription();
    }

    subscriber_type get_subscriber() const {
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

}

#endif
