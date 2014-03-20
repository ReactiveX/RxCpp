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

        struct on_next_factory
        {
            recorded_type operator()(long ticks, T value) const {
                return recorded_type(ticks, notification_type::on_next(value));
            }
        };
        struct on_completed_factory
        {
            recorded_type operator()(long ticks) const {
                return recorded_type(ticks, notification_type::on_completed());
            }
        };
        struct on_error_factory
        {
            template<class Exception>
            recorded_type operator()(long ticks, Exception&& e) const {
                return recorded_type(ticks, notification_type::on_error(std::forward<Exception>(e)));
            }
        };

        static const on_next_factory on_next;
        static const on_completed_factory on_completed;
        static const on_error_factory on_error;

        struct subscribe_factory
        {
            rxn::subscription operator()(long subscribe, long unsubscribe) const {
                return rxn::subscription(subscribe, unsubscribe);
            }
        };
        static const subscribe_factory subscribe;

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
    auto start(F&& createSource, long created, long subscribed, long unsubscribed)
        -> subscriber<T, rxt::testable_observer<T>>
    {
        typename std::decay<F>::type createSrc = std::forward<F>(createSource);

        struct state_type
            : public std::enable_shared_from_this<state_type>
        {
            typedef decltype(std::forward<F>(createSrc)()) source_type;

            std::unique_ptr<source_type> source;
            subscriber<T, rxt::testable_observer<T>> o;

            explicit state_type(subscriber<T, rxt::testable_observer<T>> o)
                : source()
                , o(o)
            {
            }
        };
        std::shared_ptr<state_type> state(new state_type(this->make_subscriber<T>()));

        schedule_absolute(created, make_action([createSrc, state](action, scheduler) {
            state->source.reset(new typename state_type::source_type(createSrc()));
            return make_action_empty();
        }));
        schedule_absolute(subscribed, make_action([state](action, scheduler) {
            state->source->subscribe(state->o);
            return make_action_empty();
        }));
        schedule_absolute(unsubscribed, make_action([state](action, scheduler) {
            state->o.unsubscribe();
            return make_action_empty();
        }));

        start();

        return state->o;
    }

    template<class T, class F>
    auto start(F&& createSource, long unsubscribed)
        -> subscriber<T, rxt::testable_observer<T>>
    {
        return start<T>(std::forward<F>(createSource), created_time, subscribed_time, unsubscribed);
    }

    template<class T, class F>
    auto start(F&& createSource)
        -> subscriber<T, rxt::testable_observer<T>>
    {
        return start<T>(std::forward<F>(createSource), created_time, subscribed_time, unsubscribed_time);
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

    template<class T>
    subscriber<T, rxt::testable_observer<T>> make_subscriber();
};

template<class T>
//static
RXCPP_SELECT_ANY const typename test::messages<T>::on_next_factory test::messages<T>::on_next = test::messages<T>::on_next_factory();

template<class T>
//static
RXCPP_SELECT_ANY const typename test::messages<T>::on_completed_factory test::messages<T>::on_completed = test::messages<T>::on_completed_factory();

template<class T>
//static
RXCPP_SELECT_ANY const typename test::messages<T>::on_error_factory test::messages<T>::on_error = test::messages<T>::on_error_factory();

template<class T>
//static
RXCPP_SELECT_ANY const typename test::messages<T>::subscribe_factory test::messages<T>::subscribe = test::messages<T>::subscribe_factory();

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

    virtual void on_subscribe(subscriber<T>) const {
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

    std::shared_ptr<mock_observer<T>> ts(new mock_observer<T>(
        std::static_pointer_cast<test>(shared_from_this())));

    return rxt::testable_observer<T>(ts, make_observer_dynamic<T>(
    // on_next
        [ts](T value)
        {
            ts->m.push_back(
                recorded_type(ts->sc->clock(), notification_type::on_next(value)));
        },
    // on_error
        [ts](std::exception_ptr e)
        {
            ts->m.push_back(
                recorded_type(ts->sc->clock(), notification_type::on_error(e)));
        },
    // on_completed
        [ts]()
        {
            ts->m.push_back(
                recorded_type(ts->sc->clock(), notification_type::on_completed()));
        }));
}

template<class T>
subscriber<T, rxt::testable_observer<T>> test::make_subscriber()
{
    auto to = this->make_observer<T>();
    return rxcpp::make_subscriber<T>(to.get_subscription(), to);
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

    virtual void on_subscribe(subscriber<T> o) const {
        sv.push_back(rxn::subscription(sc->clock()));
        auto index = sv.size() - 1;

        for (auto& message : mv) {
            auto n = message.value();
            sc->schedule_relative(message.time(), make_action([n, o](action, scheduler) {
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
    auto co = std::shared_ptr<cold_observable<T>>(new cold_observable<T>(std::static_pointer_cast<test>(shared_from_this()), std::move(messages)));
    return rxt::testable_observable<T>(co);
}

template<class T>
class hot_observable
    : public rxt::detail::test_subject_base<T>
{
    typedef hot_observable<T> this_type;
    typename test::shared sc;
    typedef rxn::recorded<typename rxn::notification<T>::type> recorded_type;
    typedef subscriber<T> observer_type;
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
