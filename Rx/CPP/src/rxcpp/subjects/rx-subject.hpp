// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_SUBJECT_HPP)
#define RXCPP_RX_SCHEDULER_SUBJECT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {


template<class T>
class multicast_observer
    : public observer_base<T>
{
    typedef observer_base<T> base;
    typedef std::vector<observer<T>> list_type;

    struct state_type;

    struct machine_type
    {
        ~machine_type()
        {
            delete state.exchange(nullptr);
        }
        machine_type(std::unique_ptr<state_type> state)
            : state(state.release())
        {
        }
        mutable std::recursive_mutex replace_lock;
        mutable std::atomic<state_type*> state;
    };

    struct state_type
    {
        virtual void spin_until_unused() const =0;
        virtual void add(observer<T> observer, machine_type& m) const =0;
        virtual void on_next(T t, machine_type& m) const =0;
        virtual void on_error(std::exception_ptr e, machine_type& m) const =0;
        virtual void on_completed(machine_type& m) const =0;
    };

    struct errored
        : public state_type
    {
        std::exception_ptr e;
        errored(std::exception_ptr e)
            : e(e)
        {
        }
        virtual void spin_until_unused() const {
        }
        virtual void add(observer<T> observer, machine_type&) const {
            observer.on_error(e);
        }
        virtual void on_next(T, machine_type&) const {
        }
        virtual void on_error(std::exception_ptr, machine_type&) const {
        }
        virtual void on_completed(machine_type&) const {
        }
    };

    struct completed
        : public state_type
    {
        virtual void spin_until_unused() const {
        }
        virtual void add(observer<T> observer, machine_type&) const {
            observer.on_completed();
        }
        virtual void on_next(T, machine_type&) const {
        }
        virtual void on_error(std::exception_ptr, machine_type&) const {
        }
        virtual void on_completed(machine_type&) const {
        }
    };

    struct casting
        : public state_type
    {
        mutable std::atomic<int> count;
        mutable list_type observers;

        struct scoped_count
        {
            std::atomic<int>& c;
            ~scoped_count()
            {
                --c;
            }
            scoped_count(std::atomic<int>& c)
                : c(c)
            {
                ++c;
            }
        };

        casting(list_type existing, observer<T> observer) 
            : observers(existing)
        {
            observers.push_back(observer);
        }

        virtual void spin_until_unused() const {
            while(count != 0);
        }
        virtual void add(observer<T> observer, machine_type& m) const {
            std::unique_ptr<state_type> that;
            {
                scoped_count track(count);
                std::unique_lock<std::recursive_mutex> guard(m.replace_lock);
                std::unique_ptr<state_type> next(new casting(observers, observer));
                state_type* compare = const_cast<casting*>(this);
                if (!m.state.compare_exchange_strong(compare, next.get())) {
                    compare->add(observer, m);
                    return;
                }
                that.reset(compare);
                next.release();
            }
            that->spin_until_unused();
        }
        virtual void on_next(T t, machine_type& m) const {
            scoped_count track(count);
            for (auto& o : observers) {
                o.on_next(std::move(t));
            }
        }
        virtual void on_error(std::exception_ptr e, machine_type& m) const {
            std::unique_ptr<state_type> that;
            {
                scoped_count track(count);
                std::unique_lock<std::recursive_mutex> guard(m.replace_lock);
                std::unique_ptr<state_type> next(new errored(e));
                state_type* compare = const_cast<casting*>(this);
                if (!m.state.compare_exchange_strong(compare, next.get())) {
                    compare->on_error(e, m);
                    return;
                }
                that.reset(compare);
                next.release();
                guard.unlock();
                for (auto& o : observers) {
                    o.on_error(std::move(e));
                }
            }
            that->spin_until_unused();
        }
        virtual void on_completed(machine_type& m) const {
            std::unique_ptr<state_type> that;
            {
                scoped_count track(count);
                std::unique_lock<std::recursive_mutex> guard(m.replace_lock);
                std::unique_ptr<state_type> next(new completed());
                state_type* compare = const_cast<casting*>(this);
                if (!m.state.compare_exchange_strong(compare, next.get())) {
                    compare->on_completed(m);
                    return;
                }
                that.reset(compare);
                next.release();
                guard.unlock();
                for (auto& o : observers) {
                    o.on_completed();
                }
            }
            that->spin_until_unused();
        }
    };

    struct empty
        : public state_type
    {
        virtual void spin_until_unused() const {
        }
        virtual void add(observer<T> observer, machine_type& m) const {
            list_type none;
            std::unique_ptr<state_type> that;
            {
                std::unique_lock<std::recursive_mutex> guard(m.replace_lock);
                std::unique_ptr<state_type> next(new casting(none, observer));
                state_type* compare = const_cast<empty*>(this);
                if (!m.state.compare_exchange_strong(compare, next.get())) {
                    compare->add(observer, m);
                    return;
                }
                that.reset(compare);
                next.release();
            }
        }
        virtual void on_next(T, machine_type&) const {
        }
        virtual void on_error(std::exception_ptr, machine_type&) const {
        }
        virtual void on_completed(machine_type&) const {
        }
    };

    typedef std::shared_ptr<machine_type> shared;
    shared machine;

public:
    multicast_observer()
        : machine(std::make_shared<machine_type>(std::unique_ptr<state_type>(new empty())))
    {
    }
    multicast_observer(composite_subscription cs)
        : observer_base<T>(std::move(cs))
        , machine(std::make_shared<machine_type>(std::unique_ptr<state_type>(new empty())))
    {
    }
    void add(observer<T> observer) const {
        machine->state.load()->add(observer, *machine);
    }
    void on_next(T t) const {
        machine->state.load()->on_next(std::move(t), *machine);
    }
    void on_error(std::exception_ptr e) const {
        machine->state.load()->on_error(std::move(e), *machine);
    }
    void on_completed() const {
        machine->state.load()->on_completed(*machine);
    }
};

}

template<class T>
class subject
{
    detail::multicast_observer<T> s;

public:
    subject()
    {
    }
    subject(composite_subscription cs)
        : s(std::move(cs))
    {
    }

    observer<T, detail::multicast_observer<T>> get_observer() const {
        return observer<T, detail::multicast_observer<T>>(s);
    }
    observable<T> get_observable() const {
        return make_dynamic_observable<T>([this](observer<T> o){
            this->s.add(std::move(o));
        });
    }
};

}

}

#endif