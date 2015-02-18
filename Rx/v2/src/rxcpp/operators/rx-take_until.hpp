// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_UNTIL_HPP)
#define RXCPP_OPERATORS_RX_TAKE_UNTIL_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class TriggerObservable, class Coordination>
struct take_until : public operator_base<T>
{
    typedef rxu::decay_t<Observable> source_type;
    typedef rxu::decay_t<TriggerObservable> trigger_source_type;
    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;
    struct values
    {
        values(source_type s, trigger_source_type t, coordination_type sf)
            : source(std::move(s))
            , trigger(std::move(t))
            , coordination(std::move(sf))
        {
        }
        source_type source;
        trigger_source_type trigger;
        coordination_type coordination;
    };
    values initial;

    take_until(source_type s, trigger_source_type t, coordination_type sf)
        : initial(std::move(s), std::move(t), std::move(sf))
    {
    }

    struct mode
    {
        enum type {
            taking,    // no messages from trigger
            clear,     // trigger completed
            triggered, // trigger sent on_next
            errored,   // error either on trigger or on observable
            stopped    // observable completed
        };
    };

    template<class Subscriber>
    void on_subscribe(Subscriber s) const {

        typedef Subscriber output_type;
        struct take_until_state_type
            : public std::enable_shared_from_this<take_until_state_type>
            , public values
        {
            take_until_state_type(const values& i, coordinator_type coor, const output_type& oarg)
                : values(i)
                , mode_value(mode::taking)
                , coordinator(std::move(coor))
                , out(oarg)
            {
                out.add(trigger_lifetime);
                out.add(source_lifetime);
            }
            typename mode::type mode_value;
            composite_subscription trigger_lifetime;
            composite_subscription source_lifetime;
            coordinator_type coordinator;
            output_type out;
        };

        auto coordinator = initial.coordination.create_coordinator(s.get_subscription());

        // take a copy of the values for each subscription
        auto state = std::make_shared<take_until_state_type>(initial, std::move(coordinator), std::move(s));

        auto trigger = on_exception(
            [&](){return state->coordinator.in(state->trigger);},
            state->out);
        if (trigger.empty()) {
            return;
        }

        auto source = on_exception(
            [&](){return state->coordinator.in(state->source);},
            state->out);
        if (source.empty()) {
            return;
        }

        auto sinkTrigger = make_subscriber<typename trigger_source_type::value_type>(
        // share parts of subscription
            state->out,
        // new lifetime
            state->trigger_lifetime,
        // on_next
            [state](const typename trigger_source_type::value_type&) {
                if (state->mode_value != mode::taking) {return;}
                state->mode_value = mode::triggered;
                state->out.on_completed();
            },
        // on_error
            [state](std::exception_ptr e) {
                if (state->mode_value != mode::taking) {return;}
                state->mode_value = mode::errored;
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (state->mode_value != mode::taking) {return;}
                state->mode_value = mode::clear;
            }
        );
        auto selectedSinkTrigger = on_exception(
            [&](){return state->coordinator.out(sinkTrigger);},
            state->out);
        if (selectedSinkTrigger.empty()) {
            return;
        }
        trigger->subscribe(std::move(selectedSinkTrigger.get()));

        auto sinkSource = make_subscriber<T>(
        // split subscription lifetime
            state->source_lifetime,
        // on_next
            [state](T t) {
                //
                // everything is crafted to minimize the overhead of this function.
                //
                if (state->mode_value < mode::triggered) {
                    state->out.on_next(t);
                }
            },
        // on_error
            [state](std::exception_ptr e) {
                if (state->mode_value > mode::clear) {return;}
                state->mode_value = mode::errored;
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                if (state->mode_value > mode::clear) {return;}
                state->mode_value = mode::stopped;
                state->out.on_completed();
            }
        );
        auto selectedSinkSource = on_exception(
            [&](){return state->coordinator.out(sinkSource);},
            state->out);
        if (selectedSinkSource.empty()) {
            return;
        }
        source->subscribe(std::move(selectedSinkSource.get()));
    }
};

template<class TriggerObservable, class Coordination>
class take_until_factory
{
    typedef rxu::decay_t<TriggerObservable> trigger_source_type;
    typedef rxu::decay_t<Coordination> coordination_type;

    trigger_source_type trigger_source;
    coordination_type coordination;
public:
    take_until_factory(trigger_source_type t, coordination_type sf)
        : trigger_source(std::move(t))
        , coordination(std::move(sf))
    {
    }
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take_until<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, trigger_source_type, Coordination>> {
        return  observable<rxu::value_type_t<rxu::decay_t<Observable>>,   take_until<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, trigger_source_type, Coordination>>(
                                                                          take_until<rxu::value_type_t<rxu::decay_t<Observable>>, Observable, trigger_source_type, Coordination>(std::forward<Observable>(source), trigger_source, coordination));
    }
};

}

template<class TriggerObservable, class Coordination>
auto take_until(TriggerObservable t, Coordination sf)
    ->      detail::take_until_factory<TriggerObservable, Coordination> {
    return  detail::take_until_factory<TriggerObservable, Coordination>(std::move(t), std::move(sf));
}

}

}

#endif
