// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_BUFFER_WITH_TIME_OR_COUNT_HPP)
#define RXCPP_OPERATORS_RX_BUFFER_WITH_TIME_OR_COUNT_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct buffer_with_time_or_count
{
    static_assert(std::is_convertible<Duration, rxsc::scheduler::clock_type::duration>::value, "Duration parameter must convert to rxsc::scheduler::clock_type::duration");
    static_assert(is_coordination<Coordination>::value, "Coordination parameter must satisfy the requirements for a Coordination");

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef rxu::decay_t<Duration> duration_type;

    struct buffer_with_time_or_count_values
    {
        buffer_with_time_or_count_values(duration_type p, int n, coordination_type c)
            : period(p)
            , count(n)
            , coordination(c)
        {
        }
        duration_type period;
        int count;
        coordination_type coordination;
    };
    buffer_with_time_or_count_values initial;

    buffer_with_time_or_count(duration_type period, int count, coordination_type coordination)
        : initial(period, count, coordination)
    {
    }

    template<class Subscriber>
    struct buffer_with_time_or_count_observer
    {
        typedef buffer_with_time_or_count_observer<Subscriber> this_type;
        typedef std::vector<T> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;

        struct buffer_with_time_or_count_subscriber_values : public buffer_with_time_or_count_values
        {
            buffer_with_time_or_count_subscriber_values(dest_type d, buffer_with_time_or_count_values v, coordinator_type c)
                : buffer_with_time_or_count_values(std::move(v))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(std::move(coordinator.get_worker()))
                , chunk_id(0)
            {
            }
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable int chunk_id;
            mutable value_type chunk;
        };
        typedef std::shared_ptr<buffer_with_time_or_count_subscriber_values> state_type;
        state_type state;

        buffer_with_time_or_count_observer(dest_type d, buffer_with_time_or_count_values v, coordinator_type c)
            : state(std::make_shared<buffer_with_time_or_count_subscriber_values>(buffer_with_time_or_count_subscriber_values(std::move(d), std::move(v), std::move(c))))
        {
            auto new_id = state->chunk_id;
            auto produce_time = state->worker.now() + state->period;
            auto localState = state;
            state->worker.schedule(produce_time, [new_id, produce_time, localState](const rxsc::schedulable&){
                produce_buffer(new_id, produce_time, localState);
            });
        }

        static void produce_buffer(int id, rxsc::scheduler::clock_type::time_point expected, state_type state) {
            if (id != state->chunk_id)
                return;

            state->dest.on_next(state->chunk);
            state->chunk.resize(0);
            auto new_id = ++state->chunk_id;
            auto produce_time = expected + state->period;
            auto localState = state;
            state->worker.schedule(produce_time, [new_id, produce_time, localState](const rxsc::schedulable&){
                produce_buffer(new_id, produce_time, localState);
            });
        }

        void on_next(T v) const {
            state->chunk.push_back(v);
            if (int(state->chunk.size()) == state->count) {
                produce_buffer(state->chunk_id, state->worker.now(), state);
            }
        }
        void on_error(std::exception_ptr e) const {
            state->dest.on_error(e);
        }
        void on_completed() const {
            state->dest.on_next(state->chunk);
            state->dest.on_completed();
        }

        static subscriber<T, observer<T, this_type>> make(dest_type d, buffer_with_time_or_count_values v) {
            auto cs = d.get_subscription();
            auto coordinator = v.coordination.create_coordinator(cs);

            return make_subscriber<T>(std::move(cs), this_type(std::move(d), std::move(v), std::move(coordinator)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(buffer_with_time_or_count_observer<Subscriber>::make(std::move(dest), initial)) {
        return      buffer_with_time_or_count_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class buffer_with_time_or_count_factory
{
    typedef rxu::decay_t<Duration> duration_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    duration_type period;
    duration_type skip;
    coordination_type coordination;
public:
    buffer_with_time_or_count_factory(duration_type p, duration_type s, coordination_type c) : period(p), skip(s), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<std::vector<rxu::value_type_t<rxu::decay_t<Observable>>>>(buffer_with_time_or_count<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, skip, coordination))) {
        return      source.template lift<std::vector<rxu::value_type_t<rxu::decay_t<Observable>>>>(buffer_with_time_or_count<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, skip, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto buffer_with_time_or_count(Duration period, int count, Coordination coordination)
    ->      detail::buffer_with_time_or_count_factory<Duration, Coordination> {
    return  detail::buffer_with_time_or_count_factory<Duration, Coordination>(period, count, coordination);
}

}

}

#endif
