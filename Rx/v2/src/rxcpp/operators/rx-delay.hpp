// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_delay_HPP)
#define RXCPP_OPERATORS_RX_delay_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct delay
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef rxu::decay_t<Duration> duration_type;

    struct delay_values
    {
        delay_values(duration_type p, coordination_type c)
            : period(p)
            , coordination(c)
        {
        }
        duration_type period;
        coordination_type coordination;
    };
    delay_values initial;

    delay(duration_type period, coordination_type coordination)
        : initial(period, coordination)
    {
    }

    template<class Subscriber>
    struct delay_observer
    {
        typedef delay_observer<Subscriber> this_type;
        typedef rxu::decay_t<T> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;

        struct delay_subscriber_values : public delay_values
        {
            delay_subscriber_values(composite_subscription cs, dest_type d, delay_values v, coordinator_type c)
                : delay_values(v)
                , cs(std::move(cs))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(coordinator.get_worker())
                , expected(worker.now())
            {
            }
            composite_subscription cs;
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            rxsc::scheduler::clock_type::time_point expected;
        };
        std::shared_ptr<delay_subscriber_values> state;

        delay_observer(composite_subscription cs, dest_type d, delay_values v, coordinator_type c)
            : state(std::make_shared<delay_subscriber_values>(delay_subscriber_values(std::move(cs), std::move(d), v, std::move(c))))
        {
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
                localState->worker.schedule(localState->worker.now() + localState->period, selectedDisposer.get());
            });
        }

        void on_next(T v) const {
            auto localState = state;
            auto work = [v, localState](const rxsc::schedulable&){
                localState->dest.on_next(v);
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(localState->worker.now() + localState->period, selectedWork.get());
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
                localState->dest.on_completed();
            };
            auto selectedWork = on_exception(
                [&](){return localState->coordinator.act(work);},
                localState->dest);
            if (selectedWork.empty()) {
                return;
            }
            localState->worker.schedule(localState->worker.now() + localState->period, selectedWork.get());
        }

        static subscriber<T, observer_type> make(dest_type d, delay_values v) {
            auto cs = composite_subscription();
            auto coordinator = v.coordination.create_coordinator();

            return make_subscriber<T>(cs, observer_type(this_type(cs, std::move(d), std::move(v), std::move(coordinator))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(delay_observer<Subscriber>::make(std::move(dest), initial)) {
        return      delay_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class delay_factory
{
    typedef rxu::decay_t<Duration> duration_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    duration_type period;
    coordination_type coordination;
public:
    delay_factory(duration_type p, coordination_type c) : period(p), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(delay<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, coordination))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(delay<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto delay(Duration period, Coordination coordination)
    ->      detail::delay_factory<Duration, Coordination> {
    return  detail::delay_factory<Duration, Coordination>(period, coordination);
}

template<class Duration>
inline auto delay(Duration period)
    ->      detail::delay_factory<Duration, identity_one_worker> {
    return  detail::delay_factory<Duration, identity_one_worker>(period, identity_current_thread());
}

}

}

#endif
