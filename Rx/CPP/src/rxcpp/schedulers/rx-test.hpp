// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_TEST_HPP)
#define RXCPP_RX_SCHEDULER_TEST_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

class test : public virtual_time<long, long>
{
public:
    typedef virtual_time<long, long> base;
    typedef base::clock_type clock_type;
    typedef std::shared_ptr<test> shared;

    static const long created_time = 100;
    static const long subscribed_time = 200;
    static const long unsubscribed_time = 1000;

    template<class T>
    struct messages
    {
        typedef typename rxn::notification<T> notification_type;
        typedef rxn::recorded<typename notification_type::type> recorded_type;

        static
        recorded_type on_next(long ticks, T value)
        {
            return recorded_type(ticks, notification_type::make_on_next(value));
        }

        static
        recorded_type on_completed(long ticks)
        {
            return recorded_type(ticks, notification_type::make_on_completed());
        }

        template<class Exception>
        static
        recorded_type on_error(long ticks, Exception e)
        {
            return recorded_type(ticks, notification_type::make_on_error(e));
        }

        static
        rxn::subscription subscribe(long subscribe, long unsubscribe)
        {
            return rxn::subscription(subscribe, unsubscribe);
        }

    private:
        ~messages();
    };

    using base::schedule_absolute;
    using base::schedule_relative;

    virtual void schedule_absolute(long when, action a)
    {
        if (when <= base::clock_now)
            when = base::clock_now + 1;

        return base::schedule_absolute(when, std::move(a));
    }

    virtual long add(long absolute, long relative)
    {
        return absolute + relative;
    }

    virtual clock_type::time_point to_time_point(long absolute)
    {
        return clock_type::time_point(clock_type::duration(absolute));
    }

    virtual long to_relative(clock_type::duration d)
    {
        return static_cast<long>(d.count());
    }

    using base::start;

    template<class T, class F>
    auto start(F createSource, long created, long subscribed, long unsubscribed)
        -> rxt::testable_observer<T>
    {
        struct state_type
        {
            typedef decltype(createSource()) source_type;
            typedef typename source_type::value_type value_type;

            explicit state_type(rxt::testable_observer<value_type> o)
                : observer(o)
            {
            }
            std::unique_ptr<source_type> source;
            rxt::testable_observer<value_type> observer;
        };
        auto state = std::make_shared<state_type>(make_observer<T>());

        schedule_absolute(created, make_action([createSource, state](action, scheduler) {
            state->source.reset(new typename state_type::source_type(createSource()));
            return make_action_empty();
        }));
        schedule_absolute(subscribed, make_action([state](action, scheduler) {
            state->source->subscribe(state->observer);
            return make_action_empty();
        }));
        schedule_absolute(unsubscribed, make_action([state](action, scheduler) {
            state->observer.unsubscribe();
            return make_action_empty();
        }));

        start();

        return state->observer;
    }

    template<class T, class F>
    auto start(F createSource, long unsubscribed)
        -> rxt::testable_observer<T>
    {
        return start<T>(std::move(createSource), created_time, subscribed_time, unsubscribed);
    }

    template<class T, class F>
    auto start(F createSource)
        -> rxt::testable_observer<T>
    {
        return start<T>(std::move(createSource), created_time, subscribed_time, unsubscribed_time);
    }

    template<class T>
    rxt::testable_observable<T> make_hot_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages);

    template<class T, size_t size>
    auto make_hot_observable(const T (&arr) [size])
        -> decltype(make_hot_observable(std::vector<T>())) {
        return      make_hot_observable(rxu::to_vector(arr));
    }

    template<class T>
    rxt::testable_observable<T> make_cold_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages);

    template<class T, size_t size>
    auto make_cold_observable(const T (&arr) [size])
        -> decltype(make_cold_observable(std::vector<T>())) {
        return      make_cold_observable(rxu::to_vector(arr));
    }

    template<class T>
    rxt::testable_observer<T> make_observer();
};

template<class T>
class mock_observer
    : public rxt::detail::test_subject_base<T>
{
    typedef typename rxn::notification<T> notification_type;
    typedef rxn::recorded<typename notification_type::type> recorded_type;

public:
    mock_observer(typename test::shared sc)
        : sc(sc)
    {
    }

    typename test::shared sc;
    std::vector<recorded_type> m;

    virtual void on_subscribe(observer<T, dynamic_observer<T>>) const {
        abort();
    }
    virtual std::vector<rxn::subscription> subscriptions() const {
        abort(); return std::vector<rxn::subscription>();
    }

    virtual std::vector<recorded_type> messages() const {
        return m;
    }
};

template<class T>
rxt::testable_observer<T> test::make_observer()
{
    typedef typename rxn::notification<T> notification_type;
    typedef rxn::recorded<typename notification_type::type> recorded_type;

    auto ts = std::make_shared<mock_observer<T>>(
        std::static_pointer_cast<test>(shared_from_this()));

    return rxt::testable_observer<T>(ts, make_observer_dynamic<T>(
    // on_next
        [ts](T value)
        {
            ts->m.push_back(
                recorded_type(ts->sc->clock(), notification_type::make_on_next(value)));
        },
    // on_error
        [ts](std::exception_ptr e)
        {
            ts->m.push_back(
                recorded_type(ts->sc->clock(), notification_type::make_on_error(e)));
        },
    // on_completed
        [ts]()
        {
            ts->m.push_back(
                recorded_type(ts->sc->clock(), notification_type::make_on_completed()));
        }));
}

template<class T>
class cold_observable
    : public rxt::detail::test_subject_base<T>
{
    typedef cold_observable<T> this_type;
    typename test::shared sc;
    typedef rxn::recorded<typename rxn::notification<T>::type> recorded_type;
    mutable std::vector<recorded_type> mv;
    mutable std::vector<rxn::subscription> sv;

public:

    cold_observable(typename test::shared sc, std::vector<recorded_type> mv)
        : sc(sc)
        , mv(std::move(mv))
    {
    }

    template<class Iterator>
    cold_observable(typename test::shared sc, Iterator begin, Iterator end)
        : scheduler(sc)
        , mv(begin, end)
    {
    }

    virtual void on_subscribe(observer<T, dynamic_observer<T>> o) const {
        sv.push_back(rxn::subscription(sc->clock()));
        auto index = sv.size() - 1;

        for (auto& message : mv) {
            auto n = message.value();
            scheduler->schedule_relative(message.time(), make_action([n, o](action, scheduler) {
                n->accept(o);
                return make_action_empty();
            }));
        }

        auto sharedThis = std::static_pointer_cast<const this_type>(this->shared_from_this());
        o.add(dynamic_subscription([sharedThis, index]() {
            sharedThis->sv[index] = rxn::subscription(sharedThis->sv[index].subscribe(), sharedThis->sc->clock());
        }));
    }

    virtual std::vector<rxn::subscription> subscriptions() const {
        return sv;
    }

    virtual std::vector<recorded_type> messages() const {
        return mv;
    }
};

template<class T>
rxt::testable_observable<T> test::make_cold_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages)
{
    return rxt::testable_observable<T>(std::make_shared<cold_observable<T>>(
        std::static_pointer_cast<test>(shared_from_this()), std::move(messages)));
}

template<class T>
class hot_observable
    : public rxt::detail::test_subject_base<T>
{
    typedef hot_observable<T> this_type;
    typename test::shared sc;
    typedef rxn::recorded<typename rxn::notification<T>::type> recorded_type;
    typedef observer<T, dynamic_observer<T>> observer_type;
    mutable std::vector<recorded_type> mv;
    mutable std::vector<rxn::subscription> sv;
    mutable std::vector<observer_type> observers;

public:

    hot_observable(typename test::shared sc, std::vector<recorded_type> mv)
        : sc(sc)
        , mv(mv)
    {
        for (auto& message : mv) {
            auto n = message.value();
            sc->schedule_absolute(message.time(), make_action([this, n](action, scheduler) {
                auto local = this->observers;
                for (auto& o : local) {
                    n->accept(o);
                }
                return make_action_empty();
            }));
        }
    }

    virtual void on_subscribe(observer_type o) const {
        observers.push_back(o);
        sv.push_back(rxn::subscription(sc->clock()));
        auto index = sv.size() - 1;

        auto sharedThis = std::static_pointer_cast<const this_type>(this->shared_from_this());
        o.add(dynamic_subscription([sharedThis, index]() {
            sharedThis->sv[index] = rxn::subscription(sharedThis->sv[index].subscribe(), sharedThis->sc->clock());
        }));
    }

    virtual std::vector<rxn::subscription> subscriptions() const {
        return sv;
    }

    virtual std::vector<recorded_type> messages() const {
        return mv;
    }
};

template<class T>
rxt::testable_observable<T> test::make_hot_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages)
{
    return rxt::testable_observable<T>(std::make_shared<hot_observable<T>>(
        std::static_pointer_cast<test>(shared_from_this()), std::move(messages)));
}

}

}

#endif