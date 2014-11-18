// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_buffer_with_time_or_count_HPP)
#define RXCPP_OPERATORS_RX_buffer_with_time_or_count_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct buffer_with_time_or_count
{
    static_assert(std::is_convertible<Duration, rxsc::scheduler::clock_type::duration>::value, "Duration parameter must convert to rxsc::scheduler::clock_type::duration");
    static_assert(is_coordination<Coordination>::value, "Coordination parameter must satisfy the requirements for a Coordination");

    typedef typename std::decay<T>::type source_value_type;
    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef typename std::decay<Duration>::type duration_type;

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
    struct buffer_with_time_or_count_observer : public buffer_with_time_or_count_values
    {
        typedef buffer_with_time_or_count_observer<Subscriber> this_type;
        typedef std::vector<T> value_type;
        typedef typename std::decay<Subscriber>::type dest_type;
        typedef observer<value_type, this_type> observer_type;

        dest_type dest;
        coordinator_type coordinator;
        rxsc::worker worker;
        mutable int number;
        mutable int chunk_id;
        mutable value_type chunk;

        buffer_with_time_or_count_observer(dest_type d, buffer_with_time_or_count_values v, coordinator_type c)
            : buffer_with_time_or_count_values(v)
            , dest(std::move(d))
            , coordinator(std::move(c))
            , worker(std::move(coordinator.get_worker()))
            , number(0)
            , chunk_id(0)
        {
            auto new_id = chunk_id;
            auto produce_time = worker.now() + period;
            worker.schedule(produce_time, [=](const rxsc::schedulable&){produce_buffer(new_id, produce_time);});
        }

        void produce_buffer(int id, rxsc::scheduler::clock_type::time_point expected) const {
            if (id != chunk_id)
                return;

            dest.on_next(chunk);
            chunk.resize(0);
            number = 0;
            auto new_id = ++chunk_id;
            auto produce_time = expected + period;
            worker.schedule(produce_time, [=](const rxsc::schedulable&){produce_buffer(new_id, produce_time);});
        }

        void on_next(T v) const {
            chunk.push_back(v);
            if (++number == count) {
                produce_buffer(chunk_id, worker.now());
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_next(chunk);
            chunk_id += 1;
            dest.on_completed();
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
    typedef typename std::decay<Duration>::type duration_type;
    typedef typename std::decay<Coordination>::type coordination_type;

    duration_type period;
    duration_type skip;
    coordination_type coordination;
public:
    buffer_with_time_or_count_factory(duration_type p, duration_type s, coordination_type c) : period(p), skip(s), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<std::vector<typename std::decay<Observable>::type::value_type>>(buffer_with_time_or_count<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, skip, coordination))) {
        return      source.template lift<std::vector<typename std::decay<Observable>::type::value_type>>(buffer_with_time_or_count<typename std::decay<Observable>::type::value_type, Duration, Coordination>(period, skip, coordination));
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
