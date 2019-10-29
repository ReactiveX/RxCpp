// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-throttle.hpp

    \brief  Return an observable that emits a value from the source and then ignores any following items until a particular timespan has passed before emitting another value.

    \tparam Duration      the type of the time interval
    \tparam Coordination  the type of the scheduler

    \param period        the period of time to suppress any emitted items after the first emission
    \param coordination  the scheduler to manage timeout for each event

    \return  Observable that emits a value from the source and then ignores any following items until a particular timespan has passed before emitting another value.

    \sample
    \snippet throttle.cpp throttle sample
    \snippet output.txt throttle sample
*/

#if !defined(RXCPP_OPERATORS_RX_THROTTLE_HPP)
#define RXCPP_OPERATORS_RX_THROTTLE_HPP

#include "../rx-includes.hpp"

#include <iostream>

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct throttle_invalid_arguments {};

template<class... AN>
struct throttle_invalid : public rxo::operator_base<throttle_invalid_arguments<AN...>> {
    using type = observable<throttle_invalid_arguments<AN...>, throttle_invalid<AN...>>;
};
template<class... AN>
using throttle_invalid_t = typename throttle_invalid<AN...>::type;

template<class T, class Duration, class Coordination>
struct throttle
{
    typedef rxu::decay_t<T> source_value_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef rxu::decay_t<Duration> duration_type;

    struct throttle_values
    {
        throttle_values(duration_type p, coordination_type c)
            : period(p)
            , coordination(c)
        {
        }

        duration_type period;
        coordination_type coordination;
    };
    throttle_values initial;

    throttle(duration_type period, coordination_type coordination)
        : initial(period, coordination)
    {
    }

    template<class Subscriber>
    struct throttle_observer
    {
        typedef throttle_observer<Subscriber> this_type;
        typedef rxu::decay_t<T> value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<T, this_type> observer_type;

        struct throttle_subscriber_values : public throttle_values
        {
            throttle_subscriber_values(composite_subscription cs, dest_type d, throttle_values v, coordinator_type c)
                : throttle_values(v)
                , cs(std::move(cs))
                , dest(std::move(d))
                , coordinator(std::move(c))
                , worker(coordinator.get_worker())
                , throttled(false)
            {
            }

            composite_subscription cs;
            dest_type dest;
            coordinator_type coordinator;
            rxsc::worker worker;
            mutable bool throttled;
        };
        typedef std::shared_ptr<throttle_subscriber_values> state_type;
        state_type state;

        throttle_observer(composite_subscription cs, dest_type d, throttle_values v, coordinator_type c)
            : state(std::make_shared<throttle_subscriber_values>(throttle_subscriber_values(std::move(cs), std::move(d), v, std::move(c))))
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

        static std::function<void(const rxsc::schedulable&)> reset_throttle(state_type state) {
            auto reset = [state](const rxsc::schedulable&) {
                state->throttled = false;
            };

            auto selectedReset = on_exception(
                    [&](){ return state->coordinator.act(reset); },
                    state->dest);
            if (selectedReset.empty()) {
                return std::function<void(const rxsc::schedulable&)>();
            }

            return std::function<void(const rxsc::schedulable&)>(selectedReset.get());
        }

        void on_next(T v) const {
            auto localState = state;

            const auto tp = localState->worker.now().time_since_epoch();
            std::cout << "on_next(" << v << ") at " << tp.count() / 1000000 << " throttled: " << (localState->throttled ? "true" : "false") << std::endl;

            if (!localState->throttled) {
                localState->throttled = true;

                state->dest.on_next(v);

                auto work = [v, localState](const rxsc::schedulable&) {
                    auto produce_time = localState->worker.now() + localState->period;

                    std::cout << "scheduling unthrottle for " << (produce_time.time_since_epoch().count() / 1000000) << std::endl;

                    localState->worker.schedule(produce_time, reset_throttle(localState));
                };
                auto selectedWork = on_exception(
                    [&](){return localState->coordinator.act(work);},
                    localState->dest);
                if (selectedWork.empty()) {
                    return;
                }
                localState->worker.schedule(selectedWork.get());
            }
        }

        void on_error(rxu::error_ptr e) const {
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

        static subscriber<T, observer_type> make(dest_type d, throttle_values v) {
            auto cs = composite_subscription();
            auto coordinator = v.coordination.create_coordinator();

            return make_subscriber<T>(cs, observer_type(this_type(cs, std::move(d), std::move(v), std::move(coordinator))));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
        -> decltype(throttle_observer<Subscriber>::make(std::move(dest), initial)) {
        return      throttle_observer<Subscriber>::make(std::move(dest), initial);
    }
};

}

/*! @copydoc rx-throttle.hpp
*/
template<class... AN>
auto throttle(AN&&... an)
    ->      operator_factory<throttle_tag, AN...> {
     return operator_factory<throttle_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<throttle_tag>
{
    template<class Observable, class Duration,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            rxu::is_duration<Duration>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Throttle = rxo::detail::throttle<SourceValue, rxu::decay_t<Duration>, identity_one_worker>>
    static auto member(Observable&& o, Duration&& d)
        -> decltype(o.template lift<SourceValue>(Throttle(std::forward<Duration>(d), identity_current_thread()))) {
        return      o.template lift<SourceValue>(Throttle(std::forward<Duration>(d), identity_current_thread()));
    }

    template<class Observable, class Coordination, class Duration,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            is_coordination<Coordination>,
            rxu::is_duration<Duration>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Throttle = rxo::detail::throttle<SourceValue, rxu::decay_t<Duration>, rxu::decay_t<Coordination>>>
    static auto member(Observable&& o, Coordination&& cn, Duration&& d)
        -> decltype(o.template lift<SourceValue>(Throttle(std::forward<Duration>(d), std::forward<Coordination>(cn)))) {
        return      o.template lift<SourceValue>(Throttle(std::forward<Duration>(d), std::forward<Coordination>(cn)));
    }

    template<class Observable, class Coordination, class Duration,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            is_coordination<Coordination>,
            rxu::is_duration<Duration>>,
        class SourceValue = rxu::value_type_t<Observable>,
        class Throttle = rxo::detail::throttle<SourceValue, rxu::decay_t<Duration>, rxu::decay_t<Coordination>>>
    static auto member(Observable&& o, Duration&& d, Coordination&& cn)
        -> decltype(o.template lift<SourceValue>(Throttle(std::forward<Duration>(d), std::forward<Coordination>(cn)))) {
        return      o.template lift<SourceValue>(Throttle(std::forward<Duration>(d), std::forward<Coordination>(cn)));
    }

    template<class... AN>
    static operators::detail::throttle_invalid_t<AN...> member(const AN&...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "throttle takes (optional Coordination, required Duration) or (required Duration, optional Coordination)");
    }
};

}

#endif
