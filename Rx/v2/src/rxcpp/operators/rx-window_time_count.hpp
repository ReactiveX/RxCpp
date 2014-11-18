// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_WINDOW_WITH_TIME_OR_COUNT_HPP)
#define RXCPP_OPERATORS_RX_WINDOW_WITH_TIME_OR_COUNT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct window_with_time_or_count
{
    typedef typename std::decay<T>::type source_value_type;
    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef typename std::decay<Duration>::type duration_type;

    struct window_with_time_or_count_values
    {
        window_with_time_or_count_values(duration_type p, int n, coordination_type c)
            : period(p)
            , count(n)
            , coordination(c)
        {
        }
        duration_type period;
        int count;
        coordination_type coordination;
    };
    window_with_time_or_count_values initial;

    window_with_time_or_count(duration_type period, int count, coordination_type coordination)
        : initial(period, count, coordination)
    {
    }

    template<class Subscriber>
    struct window_with_time_or_count_observer : public window_with_time_or_count_values, public observer_base<observable<T>>
    {
        typedef window_with_time_or_count_observer<Subscriber> this_type;
        typedef observer_base<observable<T>> base_type;
        typedef typename base_type::value_type value_type;
        typedef typename std::decay<Subscriber>::type dest_type;
        typedef observer<T, this_type> observer_type;
        dest_type dest;
        coordinator_type coordinator;
        rxsc::worker worker;
        mutable int number;
        mutable int subj_id;
        mutable rxcpp::subjects::subject<T> subj;

        window_with_time_or_count_observer(dest_type d, window_with_time_or_count_values v, coordinator_type c)
            : window_with_time_or_count_values(v)
            , dest(std::move(d))
            , coordinator(std::move(c))
            , worker(std::move(coordinator.get_worker()))
            , number(0)
            , subj_id(0)
        {
            dest.on_next(subj.get_observable().as_dynamic());
            auto new_id = subj_id;
            auto produce_time = worker.now() + period;
            worker.schedule(produce_time, [=](const rxsc::schedulable&){release_window(new_id, produce_time);});
        }

        void release_window(int id, rxsc::scheduler::clock_type::time_point expected) const {
            if (id != subj_id)
                return;

            subj.get_subscriber().on_completed();
            subj = rxcpp::subjects::subject<T>();
            dest.on_next(subj.get_observable().as_dynamic());
            number = 0;
            auto new_id = ++subj_id;
            auto produce_time = expected + period;
            worker.schedule(produce_time, [=](const rxsc::schedulable&){release_window(new_id, produce_time);});
        }

        void on_next(T v) const {
            subj.get_subscriber().on_next(v);
            if (++number == count) {
                release_window(subj_id, worker.now());
            }
        }

        void on_error(std::exception_ptr e) const {
            subj.get_subscriber().on_error(e);
            dest.on_error(e);
        }

        void on_completed() const {
            subj.get_subscriber().on_completed();
            dest.on_completed();
        }

        static subscriber<T, observer_type> make(dest_type d, window_with_time_or_count_values v) {
            auto cs = d.get_subscription();
            auto coordinator = v.coordination.create_coordinator(cs);

            return make_subscriber<T>(std::move(cs), observer_type(this_type(std::move(d), std::move(v), std::move(coordinator))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(window_with_time_or_count_observer<Subscriber>::make(std::move(dest), initial)) {
        return      window_with_time_or_count_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class window_with_time_or_count_factory
{
    typedef typename std::decay<Duration>::type duration_type;
    typedef typename std::decay<Coordination>::type coordination_type;

    duration_type period;
    int count;
    coordination_type coordination;
public:
    window_with_time_or_count_factory(duration_type p, int n, coordination_type c) : period(p), count(n), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<observable<typename std::decay<Observable>::type::value_type>>(window_with_time_or_count<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, count, coordination))) {
        return      source.template lift<observable<typename std::decay<Observable>::type::value_type>>(window_with_time_or_count<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, count, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto window_with_time_or_count(Duration period, int count, Coordination coordination)
    ->      detail::window_with_time_or_count_factory<Duration, Coordination> {
    return  detail::window_with_time_or_count_factory<Duration, Coordination>(period, count, coordination);
}

}

}

#endif
