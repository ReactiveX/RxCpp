// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_DEBOUNCE_HPP)
#define RXCPP_OPERATORS_RX_DEBOUNCE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Duration, class Coordination>
struct debounce
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef rxu::decay_t<Duration> duration_type;

    struct debounce_values
    {
        debounce_values(duration_type p, coordination_type c)
            : period(p)
            , coordination(c)
        {
        }

        duration_type period;
        coordination_type coordination;
    };
    debounce_values initial;

    debounce(duration_type period, coordination_type coordination)
        : initial(period, coordination)
    {
    }

    template<class Subscriber>
    struct debounce_observer
    {
        typedef debounce_observer<Subscriber> this_type;
        typedef rxu::decay_t<T> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;

        struct debounce_subscriber_values : public debounce_values
        {
            debounce_subscriber_values(composite_subscription cs, dest_type d, debounce_values v, coordinator_type c)
                : debounce_values(v)
                , cs(std::move(cs))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(coordinator.get_worker())
                , index(0)
            {
            }

            composite_subscription cs;
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable std::size_t index;
            mutable rxu::maybe<value_type> value;
        };
        typedef std::shared_ptr<debounce_subscriber_values> state_type;
        state_type state;

        debounce_observer(composite_subscription cs, dest_type d, debounce_values v, coordinator_type c)
            : state(std::make_shared<debounce_subscriber_values>(debounce_subscriber_values(std::move(cs), std::move(d), v, std::move(c))))
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
        }

        static std::function<void(const rxsc::schedulable&)> produce_item(std::size_t id, state_type state) {
            auto produce = [id, state](const rxsc::schedulable&) {
                if(id != state->index)
                    return;

                state->dest.on_next(*state->value);
                state->value.reset();
            };

            auto selectedProduce = on_exception(
                    [&](){ return state->coordinator.act(produce); },
                    state->dest);
            if (selectedProduce.empty()) {
                return std::function<void(const rxsc::schedulable&)>();
            }

            return std::function<void(const rxsc::schedulable&)>(selectedProduce.get());
        }

        void on_next(T v) const {
            auto localState = state;
            auto work = [v, localState](const rxsc::schedulable&) {
                auto new_id = ++localState->index;
                auto produce_time = localState->worker.now() + localState->period;

                localState->value.reset(v);
                localState->worker.schedule(produce_time, produce_item(new_id, localState));
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
            auto work = [e, localState](const rxsc::schedulable&) {
                localState->dest.on_error(e);
                localState->value.reset();
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
                if(!localState->value.empty()) {
                    localState->dest.on_next(*localState->value);
                }
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

        static subscriber<T, observer_type> make(dest_type d, debounce_values v) {
            auto cs = composite_subscription();
            auto coordinator = v.coordination.create_coordinator();

            return make_subscriber<T>(cs, observer_type(this_type(cs, std::move(d), std::move(v), std::move(coordinator))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(debounce_observer<Subscriber>::make(std::move(dest), initial)) {
        return      debounce_observer<Subscriber>::make(std::move(dest), initial);
    }
};

template<class Duration, class Coordination>
class debounce_factory
{
    typedef rxu::decay_t<Duration> duration_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    duration_type period;
    coordination_type coordination;
public:
    debounce_factory(duration_type p, coordination_type c) : period(p), coordination(c) {}
    template<class Observable>
    auto operator()(Observable&& source)
        -> decltype(source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(debounce<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, coordination))) {
        return      source.template lift<rxu::value_type_t<rxu::decay_t<Observable>>>(debounce<rxu::value_type_t<rxu::decay_t<Observable>>, Duration, Coordination>(period, coordination));
    }
};

}

template<class Duration, class Coordination>
inline auto debounce(Duration period, Coordination coordination)
    ->      detail::debounce_factory<Duration, Coordination> {
    return  detail::debounce_factory<Duration, Coordination>(period, coordination);
}

template<class Duration>
inline auto debounce(Duration period)
    ->      detail::debounce_factory<Duration, identity_one_worker> {
    return  detail::debounce_factory<Duration, identity_one_worker>(period, identity_current_thread());
}

}

}

#endif
