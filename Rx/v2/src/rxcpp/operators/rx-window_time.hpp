// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_WINDOW_WITH_TIME_HPP)
#define RXCPP_OPERATORS_RX_WINDOW_WITH_TIME_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct window_with_time
{
    typedef typename std::decay<T>::type source_value_type;
    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef typename std::decay<Duration>::type duration_type;

    struct window_with_time_values
    {
        window_with_time_values(duration_type p, duration_type s, coordination_type c)
            : period(p)
            , skip(s)
            , coordination(c)
        {
        }
        duration_type period;
        duration_type skip;
        coordination_type coordination;
    };
    window_with_time_values initial;

    window_with_time(duration_type period, duration_type skip, coordination_type coordination)
        : initial(period, skip, coordination)
    {
    }

    template<class Subscriber>
    struct window_with_time_observer : public observer_base<observable<T>>
    {
        typedef window_with_time_observer<Subscriber> this_type;
        typedef observer_base<observable<T>> base_type;
        typedef typename base_type::value_type value_type;
        typedef typename std::decay<Subscriber>::type dest_type;
        typedef observer<T, this_type> observer_type;

        struct window_with_time_subscriber_values : public window_with_time_values
        {
            window_with_time_subscriber_values(dest_type d, window_with_time_values v, coordinator_type c)
                : window_with_time_values(v)
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(std::move(coordinator.get_worker()))
                , expected(worker.now())
            {
            }
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable std::deque<rxcpp::subjects::subject<T>> subj;
            rxsc::scheduler::clock_type::time_point expected;
        };
        std::shared_ptr<window_with_time_subscriber_values> state;

        window_with_time_observer(dest_type d, window_with_time_values v, coordinator_type c)
            : state(std::make_shared<window_with_time_subscriber_values>(window_with_time_subscriber_values(std::move(d), v, std::move(c))))
        {
            auto localState = state;
            auto release_window = [localState](const rxsc::schedulable&) {
                localState->subj[0].get_subscriber().on_completed();
                localState->subj.pop_front();
            };
            auto create_window = [localState, release_window](const rxsc::schedulable&) {
                localState->subj.push_back(rxcpp::subjects::subject<T>());
                localState->dest.on_next(localState->subj[localState->subj.size() - 1].get_observable().as_dynamic());

                auto produce_at = localState->expected + localState->period;
                localState->expected += localState->skip;
                localState->worker.schedule(produce_at, release_window);
            };

            state->worker.schedule_periodically(
                state->expected,
                state->skip,
                create_window);
        }

        void on_next(T v) const {
            for (auto s : state->subj) {
                s.get_subscriber().on_next(v);
            }
        }

        void on_error(std::exception_ptr e) const {
            for (auto s : state->subj) {
                s.get_subscriber().on_error(e);
            }
            state->dest.on_error(e);
        }

        void on_completed() const {
            for (auto s : state->subj) {
                s.get_subscriber().on_completed();
            }
            state->dest.on_completed();
        }

        static subscriber<T, observer_type> make(dest_type d, window_with_time_values v) {
            auto cs = d.get_subscription();
            auto coordinator = v.coordination.create_coordinator(cs);

            return make_subscriber<T>(std::move(cs), observer_type(this_type(std::move(d), std::move(v), std::move(coordinator))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(window_with_time_observer<Subscriber>::make(std::move(dest), initial)) {
        return      window_with_time_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class window_with_time_factory
{
    typedef typename std::decay<Duration>::type duration_type;
    typedef typename std::decay<Coordination>::type coordination_type;

    duration_type period;
    duration_type skip;
    coordination_type coordination;
public:
    window_with_time_factory(duration_type p, duration_type s, coordination_type c) : period(p), skip(s), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<observable<typename std::decay<Observable>::type::value_type>>(window_with_time<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, skip, coordination))) {
        return      source.template lift<observable<typename std::decay<Observable>::type::value_type>>(window_with_time<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, skip, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto window_with_time(Duration period, Coordination coordination)
    ->      detail::window_with_time_factory<Duration, Coordination> {
    return  detail::window_with_time_factory<Duration, Coordination>(period, period, coordination);
}

template<class Duration, class Coordination>
inline auto window_with_time(Duration period, Duration skip, Coordination coordination)
    ->      detail::window_with_time_factory<Duration, Coordination> {
    return  detail::window_with_time_factory<Duration, Coordination>(period, skip, coordination);
}

}

}

#endif
