// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_SAMPLE_WITH_TIME_HPP)
#define RXCPP_OPERATORS_RX_SAMPLE_WITH_TIME_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct sample_with_time
{
    static_assert(std::is_convertible<Duration, rxsc::scheduler::clock_type::duration>::value, "Duration parameter must convert to rxsc::scheduler::clock_type::duration");
    static_assert(is_coordination<Coordination>::value, "Coordination parameter must satisfy the requirements for a Coordination");

    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef rxu::decay_t<Duration> duration_type;

    struct sample_with_time_value
    {
        sample_with_time_value(duration_type p, coordination_type c)
            : period(p)
            , coordination(c)
        {
        }
        duration_type period;
        coordination_type coordination;
    };
    sample_with_time_value initial;

    sample_with_time(duration_type period, coordination_type coordination)
        : initial(period, coordination)
    {
    }

    template<class Subscriber>
    struct sample_with_time_observer
    {
        typedef sample_with_time_observer<Subscriber> this_type;
        typedef T value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;

        struct sample_with_time_subscriber_value : public sample_with_time_value
        {
            sample_with_time_subscriber_value(composite_subscription cs, dest_type d, sample_with_time_value v, coordinator_type c)
                : sample_with_time_value(v)
                , cs(std::move(cs))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(coordinator.get_worker())
            {
            }
            composite_subscription cs;
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable rxu::maybe<value_type> value;
        };
        std::shared_ptr<sample_with_time_subscriber_value> state;

        sample_with_time_observer(composite_subscription cs, dest_type d, sample_with_time_value v, coordinator_type c)
            : state(std::make_shared<sample_with_time_subscriber_value>(sample_with_time_subscriber_value(std::move(cs), std::move(d), v, std::move(c))))
        {
            auto localState = state;

            auto disposer = [=](const rxsc::schedulable&){
                localState->cs.unsubscribe();
                localState->dest.unsubscribe();
                localState->worker.unsubscribe();
            };
            auto selectedDisposer = on_exception(
                [&](){ return localState->coordinator.act(disposer); },
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

            auto produce_sample = [localState](const rxsc::schedulable&) {
                if(!localState->value.empty()) {
                    localState->dest.on_next(*localState->value);
                    localState->value.reset();
                }
            };
            auto selectedProduce = on_exception(
                [&](){ return localState->coordinator.act(produce_sample); },
                localState->dest);
            if (selectedProduce.empty()) {
                return;
            }

            state->worker.schedule_periodically(
                localState->worker.now(),
                localState->period,
                [localState, selectedProduce](const rxsc::schedulable&) {
                    localState->worker.schedule(selectedProduce.get());
                });
        }
        
        void on_next(T v) const {
            auto localState = state;
            auto work = [v, localState](const rxsc::schedulable&) {
                localState->value.reset(v);
            };
            auto selectedWork = on_exception(
                [&](){ return localState->coordinator.act(work); },
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        void on_error(std::exception_ptr e) const {
            auto localState = state;
            auto work = [e, localState](const rxsc::schedulable&) {
                localState->dest.on_error(e);
            };
            auto selectedWork = on_exception(
                [&](){ return localState->coordinator.act(work); },
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        void on_completed() const {
            auto localState = state;
            auto work = [localState](const rxsc::schedulable&) {
                localState->dest.on_completed();
            };
            auto selectedWork = on_exception(
                [&](){ return localState->coordinator.act(work); },
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(selectedWork.get());
        }

        static subscriber<T, observer<T, this_type>> make(dest_type d, sample_with_time_value v) {
            auto cs = composite_subscription();
            auto coordinator = v.coordination.create_coordinator();

            return make_subscriber<T>(cs, this_type(cs, std::move(d), std::move(v), std::move(coordinator)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(sample_with_time_observer<Subscriber>::make(std::move(dest), initial)) {
        return      sample_with_time_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class sample_with_time_factory
{
    typedef rxu::decay_t<Duration> duration_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    duration_type period;
    coordination_type coordination;
public:
    sample_with_time_factory(duration_type p, coordination_type c) : period(p), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(sample_with_time<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, coordination))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(sample_with_time<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto sample_with_time(Duration period, Coordination coordination)
    ->      detail::sample_with_time_factory<Duration, Coordination> {
    return  detail::sample_with_time_factory<Duration, Coordination>(period, coordination);
}

template<class Duration>
inline auto sample_with_time(Duration period)
    ->      detail::sample_with_time_factory<Duration, identity_one_worker> {
    return  detail::sample_with_time_factory<Duration, identity_one_worker>(period, identity_current_thread());
}

}

}

#endif
