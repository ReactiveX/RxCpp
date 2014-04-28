// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SCHEDULER_TEST_HPP)
#define RXCPP_RX_SCHEDULER_TEST_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace schedulers {

namespace detail {

class test_type : public virtual_time<long, long>
{
public:
    typedef virtual_time<long, long> base;
    typedef base::clock_type clock_type;
    typedef std::shared_ptr<test_type> shared;

    using base::schedule_absolute;
    using base::schedule_relative;

    virtual worker get_worker() const
    {
        return base::get_worker();
    }

    virtual void schedule_absolute(long when, const schedulable& a) const
    {
        if (when <= base::clock_now)
            when = base::clock_now + 1;

        return base::schedule_absolute(when, a);
    }

    virtual long add(long absolute, long relative) const
    {
        return absolute + relative;
    }

    virtual clock_type::time_point to_time_point(long absolute) const
    {
        return clock_type::time_point(clock_type::duration(absolute));
    }

    virtual long to_relative(clock_type::duration d) const
    {
        return static_cast<long>(d.count());
    }

    using base::start;

    template<class T>
    rxt::testable_observable<T> make_hot_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages) const;

    template<class T>
    rxt::testable_observable<T> make_cold_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages) const;

    template<class T>
    subscriber<T, rxt::testable_observer<T>> make_subscriber() const;
};

template<class T>
class mock_observer
    : public rxt::detail::test_subject_base<T>
{
    typedef typename rxn::notification<T> notification_type;
    typedef rxn::recorded<typename notification_type::type> recorded_type;

public:
    mock_observer(typename test_type::shared sc)
        : sc(sc)
    {
    }

    typename test_type::shared sc;
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
subscriber<T, rxt::testable_observer<T>> test_type::make_subscriber() const
{
    typedef typename rxn::notification<T> notification_type;
    typedef rxn::recorded<typename notification_type::type> recorded_type;

    std::shared_ptr<mock_observer<T>> ts(new mock_observer<T>(std::const_pointer_cast<test_type>(std::static_pointer_cast<const test_type>(shared_from_this()))));

    return rxcpp::make_subscriber<T>(rxt::testable_observer<T>(ts, make_observer_dynamic<T>(
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
          })));
}

template<class T>
class cold_observable
    : public rxt::detail::test_subject_base<T>
{
    typedef cold_observable<T> this_type;
    typename test_type::shared sc;
    typedef rxn::recorded<typename rxn::notification<T>::type> recorded_type;
    mutable std::vector<recorded_type> mv;
    mutable std::vector<rxn::subscription> sv;

public:

    cold_observable(typename test_type::shared sc, std::vector<recorded_type> mv)
        : sc(sc)
        , mv(std::move(mv))
    {
    }

    template<class Iterator>
    cold_observable(typename test_type::shared sc, Iterator begin, Iterator end)
        : sc(sc)
        , mv(begin, end)
    {
    }

    virtual void on_subscribe(subscriber<T> o) const {
        sv.push_back(rxn::subscription(sc->clock()));
        auto index = sv.size() - 1;
        auto controller = sc->create_worker(composite_subscription());

        for (auto& message : mv) {
            auto n = message.value();
            sc->schedule_relative(message.time(), make_schedulable(
                controller,
                [n, o](const schedulable& scbl) {
                    if (o.is_subscribed()) {
                        n->accept(o);
                    }
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
rxt::testable_observable<T> test_type::make_cold_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages) const
{
    auto co = std::shared_ptr<cold_observable<T>>(new cold_observable<T>(std::const_pointer_cast<test_type>(std::static_pointer_cast<const test_type>(shared_from_this())), std::move(messages)));
    return rxt::testable_observable<T>(co);
}

template<class T>
class hot_observable
    : public rxt::detail::test_subject_base<T>
{
    typedef hot_observable<T> this_type;
    typename test_type::shared sc;
    typedef rxn::recorded<typename rxn::notification<T>::type> recorded_type;
    typedef subscriber<T> observer_type;
    mutable std::vector<recorded_type> mv;
    mutable std::vector<rxn::subscription> sv;
    mutable std::vector<observer_type> observers;

public:

    hot_observable(typename test_type::shared sc, std::vector<recorded_type> mv)
        : sc(sc)
        , mv(mv)
    {
        auto controller = sc->create_worker(composite_subscription());
        for (auto& message : mv) {
            auto n = message.value();
            sc->schedule_absolute(message.time(), make_schedulable(
                controller,
                [this, n](const schedulable& scbl) {
                    auto local = this->observers;
                    for (auto& o : local) {
                        if (o.is_subscribed()) {
                            n->accept(o);
                        }
                    }
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
rxt::testable_observable<T> test_type::make_hot_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages) const
{
    return rxt::testable_observable<T>(std::make_shared<hot_observable<T>>(
        std::const_pointer_cast<test_type>(std::static_pointer_cast<const test_type>(shared_from_this())), std::move(messages)));
}

}

class test : public scheduler
{
    detail::test_type::shared tester;
public:

    explicit test(detail::test_type::shared t)
        : scheduler(std::static_pointer_cast<scheduler_interface>(t))
        , tester(t)
    {
    }

    typedef detail::test_type::clock_type clock_type;

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

    void schedule_absolute(long when, const schedulable& a) const {
        tester->schedule_absolute(when, a);
    }

    void schedule_relative(long when, const schedulable& a) const {
        tester->schedule_relative(when, a);
    }

    template<class Arg0, class... ArgN>
    auto schedule_absolute(long when, Arg0&& a0, ArgN&&... an) const
        -> typename std::enable_if<
            (detail::is_action_function<Arg0>::value ||
            is_subscription<Arg0>::value) &&
            !is_schedulable<Arg0>::value>::type {
        tester->schedule_absolute(when, make_schedulable(tester->get_worker(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
    }

    template<class Arg0, class... ArgN>
    auto schedule_relative(long when, Arg0&& a0, ArgN&&... an) const
        -> typename std::enable_if<
            (detail::is_action_function<Arg0>::value ||
            is_subscription<Arg0>::value) &&
            !is_schedulable<Arg0>::value>::type {
        tester->schedule_relative(when, make_schedulable(tester->get_worker(), std::forward<Arg0>(a0), std::forward<ArgN>(an)...));
    }

    template<class T, class F>
    auto start(F&& createSource, long created, long subscribed, long unsubscribed) const
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

        schedule_absolute(created, [createSrc, state](const schedulable& scbl) {
            state->source.reset(new typename state_type::source_type(createSrc()));
        });
        schedule_absolute(subscribed, [state](const schedulable& scbl) {
            state->source->subscribe(state->o);
        });
        schedule_absolute(unsubscribed, [state](const schedulable& scbl) {
            state->o.unsubscribe();
        });

        tester->start();

        return state->o;
    }

    template<class T, class F>
    auto start(F&& createSource, long unsubscribed) const
        -> subscriber<T, rxt::testable_observer<T>>
    {
        return start<T>(std::forward<F>(createSource), created_time, subscribed_time, unsubscribed);
    }

    template<class T, class F>
    auto start(F&& createSource) const
        -> subscriber<T, rxt::testable_observer<T>>
    {
        return start<T>(std::forward<F>(createSource), created_time, subscribed_time, unsubscribed_time);
    }

    void start() const {
        tester->start();
    }

    template<class T>
    rxt::testable_observable<T> make_hot_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages) const{
        return tester->make_hot_observable(std::move(messages));
    }

    template<class T, size_t size>
    auto make_hot_observable(const T (&arr) [size]) const
        -> decltype(tester->make_hot_observable(std::vector<T>())) {
        return      tester->make_hot_observable(rxu::to_vector(arr));
    }

    template<class T>
    rxt::testable_observable<T> make_cold_observable(std::vector<rxn::recorded<std::shared_ptr<rxn::detail::notification_base<T>>>> messages) const {
        return tester->make_cold_observable(std::move(messages));
    }

    template<class T, size_t size>
    auto make_cold_observable(const T (&arr) [size]) const
        -> decltype(tester->make_cold_observable(std::vector<T>())) {
        return      tester->make_cold_observable(rxu::to_vector(arr));
    }

    template<class T>
    subscriber<T, rxt::testable_observer<T>> make_subscriber() const {
        return tester->make_subscriber<T>();
    }
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



inline test make_test() {
    return test(std::make_shared<detail::test_type>());
}

}

}

#endif
