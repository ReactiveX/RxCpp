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
            buffer_with_time_or_count_subscriber_values(composite_subscription cs, dest_type d, buffer_with_time_or_count_values v, coordinator_type c)
                : buffer_with_time_or_count_values(std::move(v))
                , cs(std::move(cs))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(coordinator.get_worker())
                , chunk_id(0)
            {
            }
            composite_subscription cs;
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable int chunk_id;
            mutable value_type chunk;
        };
        typedef std::shared_ptr<buffer_with_time_or_count_subscriber_values> state_type;
        state_type state;

        buffer_with_time_or_count_observer(composite_subscription cs, dest_type d, buffer_with_time_or_count_values v, coordinator_type c)
            : state(std::make_shared<buffer_with_time_or_count_subscriber_values>(buffer_with_time_or_count_subscriber_values(std::move(cs), std::move(d), std::move(v), std::move(c))))
        {
            auto new_id = state->chunk_id;
            auto produce_time = state->worker.now() + state->period;
            auto localState = state;

            auto disposer = [=](const rxsc::schedulable&){
                localState->cs.unsubscribe();
                localState->dest.unsubscribe();
                localState->worker.unsubscribe();
            };
            auto selectedDisposer = on_exception(
                [&](){return localState->coordinator.act(disposer);},
                localState->dest);
            if (selectedDisposer.empty()) {
                return;
            }

            localState->dest.add([=](){
                localState->worker.schedule(selectedDisposer.get());
            });
            localState->cs.add([=](){
                localState->worker.schedule(selectedDisposer.get());
            });

            //
            // The scheduler is FIFO for any time T. Since the observer is scheduling
            // on_next/on_error/oncompleted the timed schedule calls must be resheduled
            // when they occur to ensure that production happens after on_next/on_error/oncompleted
            //

            localState->worker.schedule(produce_time, [new_id, produce_time, localState](const rxsc::schedulable&){
                localState->worker.schedule(produce_buffer(new_id, produce_time, localState));
            });
        }

        static std::function<void(const rxsc::schedulable&)> produce_buffer(int id, rxsc::scheduler::clock_type::time_point expected, state_type state) {
            auto produce = [id, expected, state](const rxsc::schedulable&) {
                if (id != state->chunk_id)
                    return;

                state->dest.on_next(state->chunk);
                state->chunk.resize(0);
                auto new_id = ++state->chunk_id;
                auto produce_time = expected + state->period;
                state->worker.schedule(produce_time, [new_id, produce_time, state](const rxsc::schedulable&){
                    state->worker.schedule(produce_buffer(new_id, produce_time, state));
                });
            };

            auto selectedProduce = on_exception(
                [&](){return state->coordinator.act(produce);},
                state->dest);
            if (selectedProduce.empty()) {
                return std::function<void(const rxsc::schedulable&)>();
            }

            return std::function<void(const rxsc::schedulable&)>(selectedProduce.get());
        }

        void on_next(T v) const {
            auto localState = state;
            auto work = [v, localState](const rxsc::schedulable& self){
                localState->chunk.push_back(v);
                if (int(localState->chunk.size()) == localState->count) {
                    produce_buffer(localState->chunk_id, localState->worker.now(), localState)(self);
                }
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }
        void on_error(std::exception_ptr e) const {
            auto localState = state;
            auto work = [e, localState](const rxsc::schedulable&){
                localState->dest.on_error(e);
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }
        void on_completed() const {
            auto localState = state;
            auto work = [localState](const rxsc::schedulable&){
                localState->dest.on_next(localState->chunk);
                localState->dest.on_completed();
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        static subscriber<T, observer<T, this_type>> make(dest_type d, buffer_with_time_or_count_values v) {
            auto cs = composite_subscription();
            auto coordinator = v.coordination.create_coordinator();

            return make_subscriber<T>(cs, this_type(cs, std::move(d), std::move(v), std::move(coordinator)));
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
