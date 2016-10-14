// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_DEBOUNCE_HPP)
#define RXCPP_OPERATORS_RX_DEBOUNCE_HPP

#include "../rx-includes.hpp"

/*! \file rx-debounce.hpp

    \brief  Return an observable that emits an item if a particular timespan has passed without emitting another item from the source observable.

    \tparam Duration      the type of time interval
    \tparam Coordination  the type of the scheduler

    \param period        the period of time to suppress any emitted items
    \param coordination  the scheduler to manage timeout for each event

    \return  Observable that emits an item if a particular timespan has passed without emitting another item from the source observable.

    \sample
    \snippet debounce.cpp debounce sample
    \snippet output.txt debounce sample
*/

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct debounce_invalid_arguments {};

template<class... AN>
struct debounce_invalid : public rxo::operator_base<debounce_invalid_arguments<AN...>> {
    using type = observable<debounce_invalid_arguments<AN...>, debounce_invalid<AN...>>;
};
template<class... AN>
using debounce_invalid_t = typename debounce_invalid<AN...>::type;    
    
    
template <class Coordination, class Duration, class Observable>
struct debounce_traits {
    typedef rxu::decay_t<Duration> duration_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef rxu::decay_t<Observable> source_type;
    typedef typename source_type::value_type source_value_type;
    typedef source_value_type value_type;
};

template<class Coordination, class Duration, class Observable>
struct debounce : public operator_base<rxu::value_type_t<debounce_traits<Coordination, Duration, Observable>>>
{
    typedef debounce<Coordination, Duration, Observable> this_type;
    typedef debounce_traits<Coordination, Duration, Observable> traits;

    typedef typename traits::duration_type duration_type;
    typedef typename traits::coordination_type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    typedef typename traits::source_type source_type;
    typedef typename traits::source_value_type source_value_type;

    struct values
    {
        values(coordination_type c, duration_type p, source_type s)
            : coordination(std::move(c))
            , period(std::move(p))
            , source(std::move(s))
        {
        }

        coordination_type coordination;
        duration_type period;
        source_type source;
    };
    values initial;

    debounce(coordination_type coordination, duration_type period, source_type source)
        : initial(std::move(coordination), std::move(period), std::move(source))
    {
    }

    template<class State>
    void subscribe_one(std::shared_ptr<State> state) const {
        using state_type_ptr = std::shared_ptr<State>;

        composite_subscription innercs;

        state->out.add(innercs);

        auto source = on_exception(
            [&]() { return state->coordinator.in(state->source); },
            state->out);
        if (source.empty()) {
            return;
        }

        auto produce_item = [](std::size_t id, state_type_ptr st) -> std::function<void(const rxsc::schedulable&)> {
            auto produce = [id, st](const rxsc::schedulable&) {
                if(id != st->index)
                    return;

                st->out.on_next(*st->value);
                st->value.reset();
            };

            auto selectedProduce = on_exception(
                [&](){ return st->coordinator.act(produce); },
                st->out);
            if (selectedProduce.empty()) {
                return std::function<void(const rxsc::schedulable&)>();
            }

            return std::function<void(const rxsc::schedulable&)>(selectedProduce.get());
        };

        auto sink = make_subscriber<source_value_type>(
            state->out,
            innercs,
            // on_next
            [state, produce_item](source_value_type v) {
                auto localState = state;
                auto work = [v, localState, produce_item](const rxsc::schedulable&) {
                    auto new_id = ++localState->index;
                    auto produce_time = localState->worker.now() + localState->period;

                    localState->value.reset(v);
                    localState->worker.schedule(produce_time, produce_item(new_id, localState));
                };
                auto selectedWork = on_exception(
                    [&](){ return localState->coordinator.act(work); },
                    localState->out);
                if (selectedWork.empty()) {
                    return;
                }
                localState->worker.schedule(selectedWork.get());
            },
            // on_error
            [state](std::exception_ptr e) {
                auto localState = state;
                auto work = [e, localState](const rxsc::schedulable&) {
                    localState->out.on_error(e);
                    localState->value.reset();
                };
                auto selectedWork = on_exception(
                    [&](){ return localState->coordinator.act(work); },
                    localState->out);
                if (selectedWork.empty()) {
                    return;
                }
                localState->worker.schedule(selectedWork.get());
            },
            // on_completed
            [state]() {
                auto localState = state;
                auto work = [localState](const rxsc::schedulable&) {
                    if(!localState->value.empty()) {
                        localState->out.on_next(*localState->value);
                    }
                    localState->out.on_completed();
                };
                auto selectedWork = on_exception(
                    [&](){ return localState->coordinator.act(work); },
                    localState->out);
                if (selectedWork.empty()) {
                    return;
                }
                localState->worker.schedule(selectedWork.get());
            }
        );
        auto selectedSink = on_exception(
            [&](){ return state->coordinator.out(sink); },
            state->out);
        if (selectedSink.empty()) {
            return;
        }

        source->subscribe(std::move(selectedSink.get()));
    };

    template <class Subscriber>
    void on_subscribe(Subscriber scbr) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

        struct debounce_state_type
        : public std::enable_shared_from_this<debounce_state_type>,
          public values
        {
            debounce_state_type(values v, coordinator_type coor, output_type oarg)
            : values(std::move(v))
            , coordinator(std::move(coor))
            , out(std::move(oarg))
            , index(0)
            , worker(coordinator.get_worker())
            {
            }

            coordinator_type coordinator;
            output_type out;
            mutable std::size_t index;
            mutable rxu::maybe<source_value_type> value;
            rxsc::worker worker;
        };

        auto coordinator = initial.coordination.create_coordinator(scbr.get_subscription());
        auto state = std::make_shared<debounce_state_type>(initial, std::move(coordinator), std::move(scbr));
        subscribe_one(state);
    }
};

template<class... AN>
class debounce_factory
{
    using this_type = debounce_factory<AN...>;
    using tuple_type = std::tuple<rxu::decay_t<AN>...>;

    tuple_type an;

public:
    debounce_factory(tuple_type an)
    : an(std::move(an))
    {
    }

    template <class... ZN>
    auto operator()(debounce_tag, ZN&&... zn)
        -> decltype(observable_member(debounce_tag{}, std::forward<ZN>(zn)...)) {
        return      observable_member(debounce_tag{}, std::forward<ZN>(zn)...);
    }

    template <class Observable>
    auto operator()(Observable source)
        -> decltype(rxu::apply(std::tuple_cat(std::make_tuple(debounce_tag{}, source), *(tuple_type*)nullptr), *(this_type*)(nullptr)))
    {
        return      rxu::apply(std::tuple_cat(std::make_tuple(debounce_tag{}, source),                    an),                  *this);
    }
};

}

/*! @copydoc rx-debounce.hpp
*/
template<class... AN>
auto debounce(AN&&... an)
    ->      detail::debounce_factory<AN...> {
     return detail::debounce_factory<AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<debounce_tag>
{
    template<class Observable, class Duration,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            rxu::is_duration<Duration>>,
        class Debounce = rxo::detail::debounce<identity_one_worker, rxu::decay_t<Duration>, rxu::decay_t<Observable>>,
        class Value = rxu::value_type_t<Debounce>,
        class Result = observable<Value, Debounce>>
    static Result member(Observable&& o, Duration&& d)
    {
        return Result(Debounce(identity_current_thread(), std::forward<Duration>(d), std::forward<Observable>(o)));
    }

    template<class Observable, class Coordination, class Duration,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            is_coordination<Coordination>,
            rxu::is_duration<Duration>>,
        class Debounce = rxo::detail::debounce<Coordination, rxu::decay_t<Duration>, rxu::decay_t<Observable>>,
        class Value = rxu::value_type_t<Debounce>,
        class Result = observable<Value, Debounce>>
    static Result member(Observable&& o, Coordination&& cn, Duration&& d)
    {
        return Result(Debounce(std::forward<Coordination>(cn), std::forward<Duration>(d), std::forward<Observable>(o)));
    }

    template<class Observable, class Coordination, class Duration,
        class Enabled = rxu::enable_if_all_true_type_t<
            is_observable<Observable>,
            is_coordination<Coordination>,
        rxu::is_duration<Duration>>,
        class Debounce = rxo::detail::debounce<Coordination, rxu::decay_t<Duration>, rxu::decay_t<Observable>>,
        class Value = rxu::value_type_t<Debounce>,
        class Result = observable<Value, Debounce>>
    static Result member(Observable&& o, Duration&& d, Coordination&& cn)
    {
        return Result(Debounce(std::forward<Coordination>(cn), std::forward<Duration>(d), std::forward<Observable>(o)));
    }

    template<class... AN>
    static operators::detail::debounce_invalid_t<AN...> member(const AN&...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "debounce takes (optional Coordination, required Duration) or (required Duration, optional Coordination)");
    }
};

}

#endif
