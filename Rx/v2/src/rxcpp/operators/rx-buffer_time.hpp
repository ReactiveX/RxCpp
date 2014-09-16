// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_BUFFER_WITH_TIME_HPP)
#define RXCPP_OPERATORS_RX_BUFFER_WITH_TIME_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct buffer_with_time
{
    typedef typename std::decay<T>::type source_value_type;
    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef typename std::decay<Duration>::type duration_type;

    struct buffer_with_time_values
    {
        buffer_with_time_values(duration_type p, duration_type s, coordination_type c)
            : period(p)
            , skip(s)
            , coordination(c)
        {
        }
        duration_type period;
        duration_type skip;
        coordination_type coordination;
    };
    buffer_with_time_values initial;

    buffer_with_time(duration_type period, duration_type skip, coordination_type coordination)
        : initial(period, skip, coordination)
    {
    }

    template<class Subscriber>
    struct buffer_with_time_observer 
    {
        typedef buffer_with_time_observer<Subscriber> this_type;
        typedef std::vector<T> value_type;
        typedef typename std::decay<Subscriber>::type dest_type;
        typedef observer<value_type, this_type> observer_type;

        struct buffer_with_time_subscriber_values : public buffer_with_time_values
        {
            buffer_with_time_subscriber_values(dest_type d, buffer_with_time_values v, coordinator_type c)
                : buffer_with_time_values(v)
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(std::move(coordinator.get_worker()))
            {
            }
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable std::deque<value_type> chunks;
        };
        std::shared_ptr<buffer_with_time_subscriber_values> state;

        buffer_with_time_observer(dest_type d, buffer_with_time_values v, coordinator_type c)
            : state(std::make_shared<buffer_with_time_subscriber_values>(buffer_with_time_subscriber_values(std::move(d), v, std::move(c))))
        {
            auto localState = state;
            auto produce_buffer = [localState](const rxsc::schedulable& self) {
                localState->dest.on_next(std::move(localState->chunks.front()));
                localState->chunks.pop_front();
            };
            auto create_buffer = [localState, produce_buffer](const rxsc::schedulable& self) {
                localState->chunks.emplace_back();
                localState->worker.schedule(localState->worker.now() + localState->period, produce_buffer);
                self.schedule(localState->worker.now() + localState->skip);
            };

            state->chunks.emplace_back();
            state->worker.schedule(state->worker.now() + state->period, produce_buffer);
            state->worker.schedule(state->worker.now() + state->skip, create_buffer);
        }
        void on_next(T v) const {
            for(auto& chunk : state->chunks) {
                chunk.push_back(v);
            }
        }
        void on_error(std::exception_ptr e) const {
            state->dest.on_error(e);
        }
        void on_completed() const {
            auto done = on_exception(
                [&](){
                    while (!state->chunks.empty()) {
                        state->dest.on_next(std::move(state->chunks.front()));
                        state->chunks.pop_front();
                    }
                    return true;
                },
                state->dest);
            if (done.empty()) {
                return;
            }
            state->dest.on_completed();
        }

        static subscriber<T, observer<T, this_type>> make(dest_type d, buffer_with_time_values v) {
            auto cs = d.get_subscription();
            auto coordinator = v.coordination.create_coordinator(cs);

            return make_subscriber<T>(std::move(cs), this_type(std::move(d), std::move(v), std::move(coordinator)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(buffer_with_time_observer<Subscriber>::make(std::move(dest), initial)) {
        return      buffer_with_time_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class buffer_with_time_factory
{
    typedef typename std::decay<Duration>::type duration_type;
    typedef typename std::decay<Coordination>::type coordination_type;

    duration_type period;
    duration_type skip;
    coordination_type coordination;
public:
    buffer_with_time_factory(duration_type p, duration_type s, coordination_type c) : period(p), skip(s), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<std::vector<std::decay<Observable>::type::value_type>>(buffer_with_time<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, skip, coordination))) {
        return      source.template lift<std::vector<std::decay<Observable>::type::value_type>>(buffer_with_time<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, skip, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto buffer_with_time(Duration period, Coordination coordination)
    ->      detail::buffer_with_time_factory<Duration, Coordination> {
    return  detail::buffer_with_time_factory<Duration, Coordination>(period, period, coordination);
}

template<class Duration, class Coordination>
inline auto buffer_with_time(Duration period, Duration skip, Coordination coordination)
    ->      detail::buffer_with_time_factory<Duration, Coordination> {
    return  detail::buffer_with_time_factory<Duration, Coordination>(period, skip, coordination);
}

}

}

#endif
